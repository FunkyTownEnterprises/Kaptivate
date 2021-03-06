/*
 * kaptivate.cpp
 * This file is a part of Kaptivate
 * https://github.com/FunkyTownEnterprises/Kaptivate
 *
 * Copyright (c) 2011 Ben Cable, Chris Eberle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Quick overview:
 * - Raw input API - tells us which device sent us a keyboard / mouse event
 * - Keyboard hook API - lets us decide whether or not other apps see a key / mouse event
 * - Goal - decide which events we want to let through while KNOWING where they came from
 * 
 * Problems to overcome:
 *    1. Neither API can be easily used with the other
 *    2. There is NO GUARANTEED ORDER for which API will generate an event first
 */

#include "stdafx.hpp"
#include "kaptivate.hpp"
#include "kaptivate_exceptions.hpp"
#include "hooks.hpp"
#include "event_dispatcher.hpp"
#include "event_queue.hpp"
#include "scoped_mutex.hpp"

#include <iostream>
#include <assert.h>

using namespace std;
using namespace Kaptivate;

////////////////////////////////////////////////////////////////////////////////
// defines

#define MOUSE_MESSAGE    (WM_USER + 1011)
#define KEYBOARD_MESSAGE (WM_USER + 1012)
#define PING_MESSAGE     (WM_USER + 1013)
#define QUIT_MESSAGE     (WM_USER + 1014)

////////////////////////////////////////////////////////////////////////////////
// Static and extern data

extern HMODULE kaptivateDllModule;
extern HANDLE kaptivateMutex;
static KaptivateAPI* _localInstance = NULL;

////////////////////////////////////////////////////////////////////////////////
// Data passed from startCapture to the main event loop

typedef struct _kapMsgLoopParams
{
    HWND callbackWindow;
    HANDLE msgEvent;
    bool success;
} kapMsgLoopParams;


////////////////////////////////////////////////////////////////////////////////
// Creation / destruction

// Singleton static members
KaptivateAPI* KaptivateAPI::singleton = NULL;

// Constructor
KaptivateAPI::KaptivateAPI()
{
    running = false;
    suspended = false;
    rawKeyboardRunning = false;
    rawMouseRunning = false;
    userWantsMouse = false;
    userWantsKeyboard = false;

    dispatcher = new EventDispatcher();
    events = new EventQueue();

    hookCallbackWindow = 0;
    rawCallbackWindow = 0;
    hookMsgLoopThread = 0;
    rawMsgLoopThread = 0;
}

// Destructor
KaptivateAPI::~KaptivateAPI()
{
    if(isRunning())
        stopCapture();

    delete dispatcher;
    dispatcher = NULL;

    delete events;
    events = NULL;
}

// Get an instance of this thing
KaptivateAPI* KaptivateAPI::getInstance()
{
    ScopedLock lock(kaptivateMutex);

    try
    {
        if(NULL == singleton)
        {
            if(NULL == (singleton = new KaptivateAPI()))
                throw bad_alloc();
            _localInstance = singleton;
        }
    }
    catch(bad_alloc&)
    {
        throw KaptivateException("Unable to allocate memory for the Kaptivate singleton");
    }

    return singleton;
}

// Destroy the singleton
void KaptivateAPI::destroyInstance()
{
    ScopedLock lock(kaptivateMutex);
    if(singleton != NULL)
    {
        _localInstance = NULL;
        delete singleton;
        singleton = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////////
// Event processing

// Translate a raw keyboard event and stuff it into the queue
void KaptivateAPI::ProcessRawKeyboardInput(RAWINPUT* raw)
{
    bool keyUp = false;
    if((raw->data.keyboard.Flags & RI_KEY_BREAK) == RI_KEY_BREAK)
        keyUp = true;
    else if((raw->data.keyboard.Flags & RI_KEY_MAKE) != RI_KEY_MAKE)
        return;

    HANDLE device = raw->header.hDevice;
    unsigned int vkey = raw->data.keyboard.VKey;
    unsigned int scanCode = raw->data.keyboard.MakeCode;
    unsigned int message = raw->data.keyboard.Message;

    KeyboardEvent* kev = new KeyboardEvent(device, vkey, scanCode, message, keyUp);
    events->EnqueueKeyboardEvent(kev);
}

// We're being asked to interpret a keyboard hook event. Wait for the raw keyboard event, and ask the user what to
// do with it. Make a decision, and return it to the hook so that it can prevent other apps from recieving the event
// (if so desired).
LRESULT KaptivateAPI::ProcessKeyboardHook(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    unsigned int vkey = (unsigned int)wParam & 255;
    unsigned int scanCode = (((unsigned int)lParam) >> 16) & 255;

    KeyboardEvent* evt = events->DequeueKeyboardEvent();
    if(NULL == evt)
        return 0;

    LRESULT retCode = 0; // 0 means pass along
    dispatcher->handleKeyboard(*evt);
    if(evt->getDecision() == CONSUME)
        retCode = 1; // 1 means consume

    delete evt;
    return retCode;
}

// Translate a raw mouse event and stuff it into the queue
void KaptivateAPI::ProcessRawMouseInput(RAWINPUT* raw)
{
}

// We're being asked to interpret a mouse hook event. Wait for the raw mouse event, and ask the user what to do
// with it. Make a decision, and return it to the hook so that it can prevent other apps from recieving the
// event (if so desired).
LRESULT KaptivateAPI::ProcessMouseHook(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

// Our window has recieved an event from the raw api. Figure out what it is, and process it if appropriate.
void KaptivateAPI::ProcessRawInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    unsigned int pcbSize = 0;
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &pcbSize, sizeof(RAWINPUTHEADER));
    if(pcbSize == 0)
        return;

    void* buf = malloc(pcbSize);
    if(buf == NULL)
        return;
    if(pcbSize != GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buf, &pcbSize, sizeof(RAWINPUTHEADER)))
        goto cleanup;

    RAWINPUT* raw = (RAWINPUT*)buf;
    if(raw->header.dwType == RIM_TYPEMOUSE)
        ProcessRawMouseInput(raw);
    else if(raw->header.dwType == RIM_TYPEKEYBOARD)
        ProcessRawKeyboardInput(raw);

cleanup:
    free(buf);
    return;
}

// The keyboard or mouse hook has been called. This method gets called through some very special magic.
// Decide what kind of event we're responding to, and call the appropriate handler.
LRESULT KaptivateAPI::_ProcessHookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(MOUSE_MESSAGE == message)
    {
        if(suspended)
            return 0;
        return ProcessMouseHook(hWnd, wParam, lParam);
    }
    else if(KEYBOARD_MESSAGE == message)
    {
        if(suspended)
            return 0;
        return ProcessKeyboardHook(hWnd, wParam, lParam);
    }
    else if(PING_MESSAGE == message)
    {
        // Ping / Pong
        return 1;
    }
    else if(QUIT_MESSAGE == message)
    {
        PostQuitMessage(0);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT KaptivateAPI::_ProcessRawWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(WM_INPUT == message)
    {
        if(!suspended)
            ProcessRawInput(hWnd, message, wParam, lParam);
        return 0;
    }
    else if(PING_MESSAGE == message)
    {
        // Ping / Pong
        return 1;
    }
    else if(QUIT_MESSAGE == message)
    {
        PostQuitMessage(0);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////
// Main event loop (seperate thread)

// The one, the only, WndProc handler for Kaptivate. All messages from the hooks eventually wind up here.
static LRESULT CALLBACK HookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(_localInstance)
    {
        return _localInstance->_ProcessHookWndProc(hWnd, message, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

static LRESULT CALLBACK RawWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(_localInstance)
    {
        return _localInstance->_ProcessRawWndProc(hWnd, message, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

// Runs in a separate thread. Create an invisible window and process all events for that window until the window
// is told to die.
static DWORD WINAPI HookMessageLoop(LPVOID iValue)
{
    kapMsgLoopParams* params = (kapMsgLoopParams*)iValue;
    params->success = false;

    {
        // Register a window class

        WNDCLASSEX wce;

        wce.cbSize = sizeof(WNDCLASSEX);
        wce.style = CS_HREDRAW | CS_VREDRAW;
        wce.lpfnWndProc = (WNDPROC)HookWndProc;
        wce.cbClsExtra = 0;
        wce.cbWndExtra = 0;
        wce.hInstance = (HINSTANCE)kaptivateDllModule;
        wce.hIcon = NULL;
        wce.hIconSm = NULL;
        wce.hCursor = NULL;
        wce.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wce.lpszMenuName = NULL;
        wce.lpszClassName = L"KaptivateHookMsgWnd";

        if (!RegisterClassEx(&wce))
        {
            SetEvent(params->msgEvent);
            return -1;
        }

        // Set up the win32 message-only window which recieves messages
        if(NULL == (params->callbackWindow = CreateWindowEx(NULL, L"KaptivateHookMsgWnd", NULL, NULL, CW_USEDEFAULT,
            CW_USEDEFAULT, 0, 0, HWND_MESSAGE, NULL, (HINSTANCE)kaptivateDllModule, NULL)))
        {
            SetEvent(params->msgEvent);
            return -1;
        }

        // Signal the main thread that we're ready
        params->success = true;
        SetEvent(params->msgEvent);
    }

    // Finally we get to the main event loop
    MSG msg;
    BOOL bRet;
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if(bRet == -1)
        {
            break;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DestroyWindow(params->callbackWindow);

    return 0;
}


static DWORD WINAPI RawMessageLoop(LPVOID iValue)
{
    kapMsgLoopParams* params = (kapMsgLoopParams*)iValue;
    params->success = false;

    {
        // Register a window class

        WNDCLASSEX wce;

        wce.cbSize = sizeof(WNDCLASSEX);
        wce.style = CS_HREDRAW | CS_VREDRAW;
        wce.lpfnWndProc = (WNDPROC)RawWndProc;
        wce.cbClsExtra = 0;
        wce.cbWndExtra = 0;
        wce.hInstance = (HINSTANCE)kaptivateDllModule;
        wce.hIcon = NULL;
        wce.hIconSm = NULL;
        wce.hCursor = NULL;
        wce.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wce.lpszMenuName = NULL;
        wce.lpszClassName = L"KaptivateRawMsgWnd";

        if (!RegisterClassEx(&wce))
        {
            SetEvent(params->msgEvent);
            return -1;
        }

        // Set up the win32 message-only window which recieves messages
        if(NULL == (params->callbackWindow = CreateWindowEx(NULL, L"KaptivateRawMsgWnd", NULL, NULL, CW_USEDEFAULT,
            CW_USEDEFAULT, 0, 0, HWND_MESSAGE, NULL, (HINSTANCE)kaptivateDllModule, NULL)))
        {
            SetEvent(params->msgEvent);
            return -1;
        }

        // Signal the main thread that we're ready
        params->success = true;
        SetEvent(params->msgEvent);
    }

    // Finally we get to the main event loop
    MSG msg;
    BOOL bRet;
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if(bRet == -1)
        {
            break;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DestroyWindow(params->callbackWindow);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Start / Stop

// Begin capturing keyboard and / or mouse events
void KaptivateAPI::startCapture(bool wantMouse, bool wantKeyboard, bool startSuspended, UINT msgTimeoutMs)
{
    if(!wantMouse && !wantKeyboard)
        throw KaptivateException("He who wants nothing has everything");
    if(running)
        throw KaptivateException("Kaptivate is already running");
    suspended = startSuspended;
    events->start();

    {
        // Set up the hook message loop thread

        kapMsgLoopParams params;
        params.callbackWindow = 0;
        params.msgEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        params.success = false;

        // Spin up the hook thread
        DWORD threadId = 0;
        if(NULL == (this->hookMsgLoopThread = CreateThread(NULL, 0, HookMessageLoop, &params, 0, &threadId)))
            throw KaptivateException("Failed to create the hook loop thread loop");

        // Wait for the thread to decide whether or not everything is good
        WaitForSingleObject(params.msgEvent, INFINITE);
        CloseHandle (params.msgEvent);
        if(!params.success)
            throw KaptivateException("Failed to initialize the hook message window");
        this->hookCallbackWindow = params.callbackWindow;
        if(!pingMessageWindow(this->hookCallbackWindow))
            throw KaptivateException("Something horrible has happened");
    }

    {
        // Set up the raw message loop thread

        kapMsgLoopParams params;
        params.callbackWindow = 0;
        params.msgEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        params.success = false;

        // Spin up the raw thread
        DWORD threadId = 0;
        if(NULL == (this->rawMsgLoopThread = CreateThread(NULL, 0, RawMessageLoop, &params, 0, &threadId)))
            throw KaptivateException("Failed to create the raw loop thread loop");

        // Wait for the thread to decide whether or not everything is good
        WaitForSingleObject(params.msgEvent, INFINITE);
        CloseHandle (params.msgEvent);
        if(!params.success)
            throw KaptivateException("Failed to initialize the raw message window");
        this->rawCallbackWindow = params.callbackWindow;
        if(!pingMessageWindow(this->rawCallbackWindow))
            throw KaptivateException("Something horrible has happened");
    }

    // Finally set up the hooks
    try
    {
        if(!startSuspended)
        {
            if(!startRawCapture(wantMouse, wantKeyboard))
                throw KaptivateException("Failed to start raw capture");
        }

        short ss = (startSuspended) ? 1 : 0;
        if(0 != kaptivateHookInit(this->hookCallbackWindow, KEYBOARD_MESSAGE,
                                  MOUSE_MESSAGE, msgTimeoutMs, ss))
            throw KaptivateException("Failed to initialize the hooks");
    }
    catch(...)
    {
        kaptivateHookUninit();
        stopRawCapture();
        tryStopMsgLoop();
        throw;
    }

    userWantsMouse = wantMouse;
    userWantsKeyboard = wantKeyboard;
    running = true;
}

// Stop capturing keyboard and mouse events
void KaptivateAPI::stopCapture()
{
    if(!running)
        throw KaptivateException("Kaptivate is not running");

    // First stop the event queue
    events->stop();

    // Next stop any further messages from being generated
    if(0 != kaptivateHookUninit())
        throw KaptivateException("Failed to uninitialize the hooks");

    // Stop the raw events from coming in
    if(!stopRawCapture())
        throw KaptivateException("Failed to stop raw capture");

    // Attempt to shut down the message window
    if(!tryStopMsgLoop())
        throw KaptivateException("Failed to stop the Kaptivate message loop");

    running = false;
    userWantsMouse = false;
    userWantsKeyboard = false;
}

// Attempt to stop the message loop thread
bool KaptivateAPI::tryStopMsgLoop()
{
    // Tell the hook window we're done
    DWORD res = 0;
    if(0 == SendMessageTimeout(this->hookCallbackWindow, QUIT_MESSAGE, 0, 0, SMTO_ABORTIFHUNG, 5000, &res))
        return false;

    // Wait for the hook thread to return
    if(WAIT_OBJECT_0 != WaitForSingleObject(this->hookMsgLoopThread, 5000))
        return false;

    // Tell the raw window we're done
    res = 0;
    if(0 == SendMessageTimeout(this->rawCallbackWindow, QUIT_MESSAGE, 0, 0, SMTO_ABORTIFHUNG, 5000, &res))
        return false;

    // Wait for the raw thread to return
    if(WAIT_OBJECT_0 != WaitForSingleObject(this->rawMsgLoopThread, 5000))
        return false;

    return true;
}

// Register for raw input events
bool KaptivateAPI::startRawCapture(bool wantMouse, bool wantKeyboard)
{
    if(rawKeyboardRunning || rawMouseRunning)
        throw KaptivateException("The raw capture events are still running");

    unsigned int ct = 0;
    if(wantKeyboard) ++ct;
    if(wantMouse) ++ct;

    // Should never happen
    if(ct == 0)
        return false;

    RAWINPUTDEVICE* rid = new RAWINPUTDEVICE[ct];
    ct = 0;

    // Register for keyboard events in the background
    if(wantKeyboard)
    {
        rid[ct].usUsagePage = 0x01; // It's a keyboard
        rid[ct].usUsage = 0x06;
        rid[ct].dwFlags = RIDEV_INPUTSINK | RIDEV_NOLEGACY | RIDEV_NOHOTKEYS;
        rid[ct].hwndTarget = this->rawCallbackWindow;
        ++ct;
    }

    // Register for mouse events in the background
    if(wantMouse)
    {
        rid[ct].usUsagePage = 0x01; // It's a mouse
        rid[ct].usUsage = 0x02;
        rid[ct].dwFlags = RIDEV_INPUTSINK | RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE;
        rid[ct].hwndTarget = this->rawCallbackWindow;
        ++ct;
    }

    BOOL ret = RegisterRawInputDevices(rid, ct, sizeof(RAWINPUTDEVICE));
    delete[] rid;

    if(!ret)
        return false;

    rawKeyboardRunning = wantKeyboard;
    rawMouseRunning = wantMouse;

    return true;
}

// Unregister all raw input events
bool KaptivateAPI::stopRawCapture()
{
    if(!rawKeyboardRunning && !rawMouseRunning)
        return true;

    unsigned int ct = 0;
    if(rawKeyboardRunning) ++ct;
    if(rawMouseRunning) ++ct;

    // Should never happen
    if(ct == 0)
        return false;

    RAWINPUTDEVICE* rid = new RAWINPUTDEVICE[ct];
    ct = 0;

    // Register for keyboard events in the background
    if(rawKeyboardRunning)
    {
        rid[ct].usUsagePage = 0x01;
        rid[ct].usUsage = 0x06; // It's a keyboard
        rid[ct].dwFlags = RIDEV_REMOVE;
        rid[ct].hwndTarget = 0x0;
        ++ct;
    }

    // Register for mouse events in the background
    if(rawMouseRunning)
    {
        rid[ct].usUsagePage = 0x01;
        rid[ct].usUsage = 0x02; // It's a mouse
        rid[ct].dwFlags = RIDEV_REMOVE;
        rid[ct].hwndTarget = 0x0;
        ++ct;
    }

    // Actually we're unregistering them.
    BOOL ret = RegisterRawInputDevices(rid, ct, sizeof(RAWINPUTDEVICE));
    delete[] rid;

    if(!ret)
        return false;

    rawKeyboardRunning = false;
    rawMouseRunning = false;

    return true;
}

// Ensure that the message window is alive and kicking
bool KaptivateAPI::pingMessageWindow(HWND wnd) const
{
    LRESULT res;
    DWORD dwres = 0;

    while(true)
    {
        res = SendMessageTimeout(wnd, PING_MESSAGE, 0, 0, SMTO_ABORTIFHUNG, 5000, &dwres);
        if(0 == res)
        {
            if(ERROR_TIMEOUT == GetLastError())
                return false;
        }
        else if(1 == res)
        {
            return true;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// Suspend / Resume

// Keep our hooks alive, but temporarily suspend capturing the events
void KaptivateAPI::suspendCapture()
{
    if(!running)
        throw KaptivateException("Kaptivate is not currently running");
    if(suspended)
        throw KaptivateException("Kaptivate is already suspended");
    if(0 != kaptivateHookPause())
        throw KaptivateException("Unable to suspend hook processing");
    suspended = true;
}

// Assume that our hooks are alive, and resume capturing events
void KaptivateAPI::resumeCapture()
{
    if(!running)
        throw KaptivateException("Kaptivate is not currently running");
    if(!suspended)
        throw KaptivateException("Kaptivate is already running");
    suspended = true;
    if(0 != kaptivateHookUnpause())
        throw KaptivateException("Unable to resume hook processing");
}


////////////////////////////////////////////////////////////////////////////////
// Status

// Is Kaptivate initialized and capturing events?
bool KaptivateAPI::isRunning() const
{
    if(!running)
        return false;
    if(!pingMessageWindow(this->hookCallbackWindow))
        return false;
    if(!pingMessageWindow(this->rawCallbackWindow))
        return false;
    return true;
}

// Is Kaptivate paused?
bool KaptivateAPI::isSuspended() const
{
    return suspended;
}


////////////////////////////////////////////////////////////////////////////////
// Device enumeration

// Get a list of all attached keyboards
vector<KeyboardInfo> KaptivateAPI::enumerateKeyboards()
{
    return dispatcher->enumerateKeyboards();
}

// Get a list of all attached mice
vector<MouseInfo> KaptivateAPI::enumerateMice()
{
    return dispatcher->enumerateMice();
}


////////////////////////////////////////////////////////////////////////////////
// Event handler registration / unregistration

// Tell kaptivate that you're interested in processing messages from a particular keyboard
void KaptivateAPI::registerKeyboardHandler(string idRegex, KeyboardHandler* handler)
{
    dispatcher->registerKeyboardHandler(idRegex, handler);
}

// Tell kaptivate that you're interested in processing messages from a particular mouse
void KaptivateAPI::resgisterMouseHandler(string idRegex, MouseHandler* handler)
{
    dispatcher->resgisterMouseHandler(idRegex, handler);
}

// Tell kaptivate that a particular keyboard handler is going away
void KaptivateAPI::unregisterKeyboardHandler(KeyboardHandler* handler)
{
    dispatcher->unregisterKeyboardHandler(handler);
}

// Tell kaptivate that a particular mouse handler is going away
void KaptivateAPI::unregisterMouseHandler(MouseHandler* handler)
{
    dispatcher->unregisterMouseHandler(handler);
}


////////////////////////////////////////////////////////////////////////////////
// Keyboard Event

KeyboardEvent::KeyboardEvent(HANDLE device, unsigned int vkey, unsigned int scanCode,
                             unsigned int wmMessage, bool keyUp)
{
    this->decision = UNDECIDED;
    this->deviceHandle = device;
    this->vkey = vkey;
    this->scanCode = scanCode;
    this->wmMessage = wmMessage;
    this->keyUp = keyUp;
    this->info = NULL;
}

KeyboardEvent::~KeyboardEvent()
{
}

HANDLE KeyboardEvent::getDeviceHandle() const
{
    return this->deviceHandle;
}

KeyboardInfo* KeyboardEvent::getDeviceInfo() const
{
    return this->info;
}

void KeyboardEvent::setDeviceInfo(KeyboardInfo* kbdInfo)
{
    if(!this->info)
    {
        this->info = kbdInfo;
    }
}

unsigned int KeyboardEvent::getVkey() const
{

    return vkey;
}

unsigned int KeyboardEvent::getScanCode() const
{
    return scanCode;
}

unsigned int KeyboardEvent::getWindowMessage() const
{
    return wmMessage;
}

bool KeyboardEvent::getKeyUp() const
{
    return keyUp;
}

Decision KeyboardEvent::getDecision() const
{
    return decision;
}

void KeyboardEvent::setDecision(Decision decision)
{
    this->decision = decision;
}


////////////////////////////////////////////////////////////////////////////////
// Mouse Button Event

MouseButtonEvent::MouseButtonEvent(HANDLE device)
{
    this->decision = UNDECIDED;
    this->deviceHandle = device;
    this->info = NULL;
}

MouseButtonEvent::~MouseButtonEvent()
{
}

HANDLE MouseButtonEvent::getDeviceHandle() const
{
    return this->deviceHandle;
}

MouseInfo* MouseButtonEvent::getDeviceInfo() const
{
    return this->info;
}

void MouseButtonEvent::setDeviceInfo(MouseInfo* mouseInfo)
{
    if(!this->info)
    {
        this->info = mouseInfo;
    }
}

Decision MouseButtonEvent::getDecision() const
{
    return decision;
}

void MouseButtonEvent::setDecision(Decision decision)
{
    this->decision = decision;
}


////////////////////////////////////////////////////////////////////////////////
// Mouse Wheel Event

MouseWheelEvent::MouseWheelEvent(HANDLE device)
{
    this->decision = UNDECIDED;
    this->deviceHandle = device;
    this->info = NULL;
}

MouseWheelEvent::~MouseWheelEvent()
{
}

HANDLE MouseWheelEvent::getDeviceHandle() const
{
    return this->deviceHandle;
}

MouseInfo* MouseWheelEvent::getDeviceInfo() const
{
    return this->info;
}

void MouseWheelEvent::setDeviceInfo(MouseInfo* mouseInfo)
{
    if(!this->info)
    {
        this->info = mouseInfo;
    }
}

Decision MouseWheelEvent::getDecision() const
{
    return decision;
}

void MouseWheelEvent::setDecision(Decision decision)
{
    this->decision = decision;
}


////////////////////////////////////////////////////////////////////////////////
// Mouse Move Event

MouseMoveEvent::MouseMoveEvent(HANDLE device)
{
    this->decision = UNDECIDED;
    this->deviceHandle = device;
    this->info = NULL;
}

MouseMoveEvent::~MouseMoveEvent()
{
}

HANDLE MouseMoveEvent::getDeviceHandle() const
{
    return this->deviceHandle;
}

MouseInfo* MouseMoveEvent::getDeviceInfo() const
{
    return this->info;
}

void MouseMoveEvent::setDeviceInfo(MouseInfo* mouseInfo)
{
    if(!this->info)
    {
        this->info = mouseInfo;
    }
}

Decision MouseMoveEvent::getDecision() const
{
    return decision;
}

void MouseMoveEvent::setDecision(Decision decision)
{
    this->decision = decision;
}
