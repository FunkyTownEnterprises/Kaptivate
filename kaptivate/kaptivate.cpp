/*
 * Kaptivate.cpp
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

#include "stdafx.h"
#include "kaptivate.h"
#include "kaptivate_exceptions.h"
#include "hooks.h"

#include <iostream>
using namespace std;
using namespace Kaptivate;


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

KaptivateAPI::KaptivateAPI()
{
    running   = false;
    suspended = false;
    msgLoopThread = 0;
}

KaptivateAPI::~KaptivateAPI()
{
    if(isRunning())
        stopCapture();
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

static UINT kaptivateKeyboardMessage = 0, kaptivateMouseMessage = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main event loop (seperate thread)

extern HMODULE kaptivateDllModule;
static DWORD WINAPI MessageLoop(LPVOID iValue)
{
    {
        kapMsgLoopParams* params = (kapMsgLoopParams*)iValue;
        params->success = false;

        // Register a window class

        WNDCLASS wc;
        wc.style = 0;
        wc.lpfnWndProc = (WNDPROC)WndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = (HINSTANCE)kaptivateDllModule;
        wc.hIcon = 0;
        wc.hCursor = 0;
        wc.hbrBackground = 0;
        wc.lpszMenuName = 0;
        wc.lpszClassName = L"KaptivateWndClass";

        if (!RegisterClass(&wc))
        {
            SetEvent(params->msgEvent);
            return -1;
        }

        // Set up the win32 message-only window which recieves messages
        if(NULL == (params->callbackWindow = CreateWindowEx(0, 0, L"KaptivateWndClass", 0, 0,
            0, 0, 0, HWND_MESSAGE, NULL, (HINSTANCE)kaptivateDllModule, (LPVOID)NULL)))
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

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Start / Stop

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

    // Finally set up the hooks
    try
    {
        this->callbackWindow = params.callbackWindow;
        short ss = (startSuspended) ? 1 : 0;
        if(0 != kaptivateHookInit(this->callbackWindow, kaptivateKeyboardMessage,
                                  kaptivateMouseMessage, msgTimeoutMs, ss))
            throw KaptivateException("Failed to initialize the hooks");
    }
    catch(...)
    {
        tryStopMsgLoop();
        throw;
    }

    running = true;
}

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
}

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


////////////////////////////////////////////////////////////////////////////////
// Suspend / Resume

void KaptivateAPI::suspendCapture()
{
    if(0 != kaptivateHookPause())
        throw KaptivateException("Please call startCapture first");
    suspended = true;
}

void KaptivateAPI::resumeCapture()
{
    if(0 != kaptivateHookUnpause())
        throw KaptivateException("Please call startCapture first");
    suspended = false;
}


////////////////////////////////////////////////////////////////////////////////
// Status

bool KaptivateAPI::isRunning() const
{
    // TODO: Ping the window
    return running;
}

bool KaptivateAPI::isSuspended() const
{
    return suspended;
}


////////////////////////////////////////////////////////////////////////////////
// Device enumeration

vector<KeyboardInfo> KaptivateAPI::enumerateKeyboards()
{
    vector<KeyboardInfo> dummy;
    return dummy;
}

vector<MouseInfo> KaptivateAPI::enumerateMice()
{
    vector<MouseInfo> dummy;
    return dummy;
}


////////////////////////////////////////////////////////////////////////////////
// Event handler registration / unregistration

void KaptivateAPI::registerKeyboardHandler(string idRegex, KeyboardHandler* handler)
{
}

void KaptivateAPI::resgisterMouseHandler(string idRegex, MouseHandler* handler)
{
}

void KaptivateAPI::unregisterKeyboardHandler(KeyboardHandler* handler)
{
}

void KaptivateAPI::unregisterMouseHandler(MouseHandler* handler)
{
}
