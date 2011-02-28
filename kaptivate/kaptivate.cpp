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

#include "stdafx.h"
#include "kaptivate.h"
#include "kaptivate_exceptions.h"
#include "hooks.h"
#include "device_handler_map.h"

#include <iostream>
using namespace std;
using namespace Kaptivate;

////////////////////////////////////////////////////////////////////////////////
// Static and extern data

extern HMODULE kaptivateDllModule;
static UINT kaptivateKeyboardMessage = 0, kaptivateMouseMessage = 0, kaptivatePingMessage = 0;
static bool kaptivateSuspendProcessing = false;
static DeviceHandlerMap* kaptivateHandler = NULL;

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
bool KaptivateAPI::instanceFlag       = false;
KaptivateAPI* KaptivateAPI::singleton = NULL;

// Constructor
KaptivateAPI::KaptivateAPI()
{
    running   = false;
    rawKeyboardRunning = false;
    rawMouseRunning = false;
    userWantsMouse = false;
    userWantsKeyboard = false;
    msgLoopThread = 0;
    kaptivateHandler = new DeviceHandlerMap();
}

// Destructor
KaptivateAPI::~KaptivateAPI()
{
    if(isRunning())
        stopCapture();
    delete kaptivateHandler;
    kaptivateHandler = NULL;
}

// Get an instance of this thing
KaptivateAPI* KaptivateAPI::getInstance()
{
    if(!instanceFlag)
    {
        try
        {
            if(NULL == (singleton = new KaptivateAPI()))
                throw bad_alloc();
        }
        catch(bad_alloc&)
        {
            throw KaptivateException("Unable to allocate memory for the Kaptivate singleton");
        }

        instanceFlag = true;
    }

    return singleton;
}

// Destroy the singleton
void KaptivateAPI::destroyInstance()
{
    if(!instanceFlag || singleton == NULL)
        return;

    delete singleton;
    singleton = NULL;

    instanceFlag = false;
}


////////////////////////////////////////////////////////////////////////////////
// Event processing

static void ProcessRawKeyboardInput(RAWINPUT* raw)
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

    // TODO: Enqueue this event and return
}

static void ProcessRawMouseInput(RAWINPUT* raw)
{
}

static void ProcessRawInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(WM_INPUT == message)
    {
        if(!kaptivateSuspendProcessing)
            ProcessRawInput(hWnd, message, wParam, lParam);
        return 0;
    }
    else if(kaptivateMouseMessage == message)
    {
        if(kaptivateSuspendProcessing)
            return 0;

        // TODO: Handle the mouse message here
        //return 0;
    }
    else if(kaptivateKeyboardMessage == message)
    {
        if(kaptivateSuspendProcessing)
            return 0;

        // TODO: Handle the keyboard message here
        //kaptivateHandler->handleKeyboard(...);
        //if(evt.action == CONSUME)
        //    return 1; // > 0 means consume
        //else
        //    return 0; // 0 means pass
    }
    else if(kaptivatePingMessage == message)
    {
        // Ping / Pong
        return 1;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}


////////////////////////////////////////////////////////////////////////////////
// Main event loop (seperate thread)

static DWORD WINAPI MessageLoop(LPVOID iValue)
{
    {
        kapMsgLoopParams* params = (kapMsgLoopParams*)iValue;
        params->success = false;

        // Register a window class

        WNDCLASSEX wce;

        wce.cbSize = sizeof(WNDCLASSEX);
        wce.style = CS_HREDRAW | CS_VREDRAW;
        wce.lpfnWndProc = (WNDPROC)WndProc;
        wce.cbClsExtra = 0;
        wce.cbWndExtra = 0;
        wce.hInstance = (HINSTANCE)kaptivateDllModule;
        wce.hIcon = NULL;
        wce.hIconSm = NULL;
        wce.hCursor = NULL;
        wce.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        wce.lpszMenuName = NULL;
        wce.lpszClassName = L"KaptivateMsgWnd";

        if (!RegisterClassEx(&wce))
        {
            SetEvent(params->msgEvent);
            return -1;
        }

        // Set up the win32 message-only window which recieves messages
        if(NULL == (params->callbackWindow = CreateWindowEx(NULL, L"KaptivateMsgWnd", NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
            HWND_MESSAGE, NULL, (HINSTANCE)kaptivateDllModule, NULL)))
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
    while (0 != GetMessage (&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage (&msg);
    }

    {
        kapMsgLoopParams* params = (kapMsgLoopParams*)iValue;
        DestroyWindow(params->callbackWindow);
    }

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

    // Find out what our messaging is going to look like
    kaptivateMouseMessage = 0;
    kaptivateKeyboardMessage = 0;
    if(wantMouse)
        kaptivateMouseMessage = RegisterWindowMessage(L"6F3DB758-492B-4693-BC23-45F6A44C9625"); // just some random guid
    if(wantKeyboard)
        kaptivateKeyboardMessage = RegisterWindowMessage(L"FF2FD0A6-C41C-463c-94D8-1AD852C57E74"); // another random guid

    kaptivatePingMessage = RegisterWindowMessage(L"F77D4A67-0F92-44d6-B1DF-24264F4CD97C"); // oh but this one's special...
                                                                                           // random, but special... to me.
    kaptivateSuspendProcessing = startSuspended;

    // Set up the data we're going to pass in
    kapMsgLoopParams params;
    params.callbackWindow = 0;
    params.msgEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    params.success = false;

    // Spin up the thread
    DWORD threadId = 0;
    if(NULL == (this->msgLoopThread = CreateThread(NULL, 0, MessageLoop, &params, 0, &threadId)))
        throw KaptivateException("Failed to create the main thread loop");

    // Wait for the thread to decide whether or not everything is good
    WaitForSingleObject(params.msgEvent, INFINITE);
    CloseHandle (params.msgEvent);
    if(!params.success)
        throw KaptivateException("Failed to initialize the message window");
    this->callbackWindow = params.callbackWindow;
    if(!pingMessageWindow())
        throw KaptivateException("Something horrible has happened");

    // Finally set up the hooks
    try
    {
        if(!startSuspended)
        {
            if(!startRawCapture(wantMouse, wantKeyboard))
                throw KaptivateException("Failed to start raw capture");
        }

        short ss = (startSuspended) ? 1 : 0;
        if(0 != kaptivateHookInit(this->callbackWindow, kaptivateKeyboardMessage,
                                  kaptivateMouseMessage, msgTimeoutMs, ss))
            throw KaptivateException("Failed to initialize the hooks");
    }
    catch(...)
    {
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

    // First stop any further messages from being generated
    if(0 != kaptivateHookUninit())
        throw KaptivateException("Failed to uninitialize the hooks");

    // Attempt to shut down the message window
    if(!tryStopMsgLoop())
        throw KaptivateException("Failed to stop the main Kaptivate message loop");

    running = false;
    userWantsMouse = false;
    userWantsKeyboard = false;
}

// Attempt to stop the message loop thread
bool KaptivateAPI::tryStopMsgLoop()
{
    // Tell the window we're done
    DWORD res = 0;
    if(0 == SendMessageTimeout(this->callbackWindow, WM_QUIT, 0, 0, SMTO_ABORTIFHUNG, 5000, &res))
        return false;

    // Wait for the thread to return
    // TODO: Time out
    WaitForSingleObject(msgLoopThread, INFINITE);

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
        rid[ct].hwndTarget = this->callbackWindow;
        ++ct;
    }

    // Register for mouse events in the background
    if(wantMouse)
    {
        rid[ct].usUsagePage = 0x01; // It's a mouse
        rid[ct].usUsage = 0x02;
        rid[ct].dwFlags = RIDEV_INPUTSINK | RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE;
        rid[ct].hwndTarget = this->callbackWindow;
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
bool KaptivateAPI::pingMessageWindow() const
{
    LRESULT res;
    DWORD dwres = 0;

    while(true)
    {
        res = SendMessageTimeout(this->callbackWindow, kaptivatePingMessage, 0, 0, SMTO_ABORTIFHUNG, 5000, &dwres);
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
    if(kaptivateSuspendProcessing)
        throw KaptivateException("Kaptivate is already suspended");
    if(0 != kaptivateHookPause())
        throw KaptivateException("Unable to suspend hook processing");
    kaptivateSuspendProcessing = true;
}

// Assume that our hooks are alive, and resume capturing events
void KaptivateAPI::resumeCapture()
{
    if(!running)
        throw KaptivateException("Kaptivate is not currently running");
    if(!kaptivateSuspendProcessing)
        throw KaptivateException("Kaptivate is already running");
    kaptivateSuspendProcessing = true;
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
    if(!pingMessageWindow())
        return false;
    return true;
}

// Is Kaptivate paused?
bool KaptivateAPI::isSuspended() const
{
    return kaptivateSuspendProcessing;
}


////////////////////////////////////////////////////////////////////////////////
// Device enumeration

vector<KeyboardInfo> KaptivateAPI::enumerateKeyboards()
{
    return kaptivateHandler->enumerateKeyboards();
}

vector<MouseInfo> KaptivateAPI::enumerateMice()
{
    return kaptivateHandler->enumerateMice();
}


////////////////////////////////////////////////////////////////////////////////
// Event handler registration / unregistration

void KaptivateAPI::registerKeyboardHandler(string idRegex, KeyboardHandler* handler)
{
    kaptivateHandler->registerKeyboardHandler(idRegex, handler);
}

void KaptivateAPI::resgisterMouseHandler(string idRegex, MouseHandler* handler)
{
    kaptivateHandler->resgisterMouseHandler(idRegex, handler);
}

void KaptivateAPI::unregisterKeyboardHandler(KeyboardHandler* handler)
{
    kaptivateHandler->unregisterKeyboardHandler(handler);
}

void KaptivateAPI::unregisterMouseHandler(MouseHandler* handler)
{
    kaptivateHandler->unregisterMouseHandler(handler);
}
