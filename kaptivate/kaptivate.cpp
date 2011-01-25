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
// Main event loop (seperate thread)

static DWORD WINAPI MessageLoop(LPVOID iValue)
{
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
    UINT keyboardMessage = 0, mouseMessage = 0;
    if(wantMouse)
        mouseMessage = RegisterWindowMessage(L"6F3DB758-492B-4693-BC23-45F6A44C9625"); // just some random guid
    if(wantKeyboard)
        keyboardMessage = RegisterWindowMessage(L"FF2FD0A6-C41C-463c-94D8-1AD852C57E74"); // another random guid

    // TODO: Set up the win32 window here
    HWND callbackWindow = 0;

    DWORD threadId = 0;
    if(NULL == (msgLoopThread = CreateThread(NULL, 0, MessageLoop, NULL, 0, &threadId)))
        throw KaptivateException("Failed to create the main thread loop");

    // TODO: Ping the window until it comes up

    // Finally set up the hooks
    if(0 != kaptivateHookInit(callbackWindow, keyboardMessage, mouseMessage, msgTimeoutMs, 0))
        throw KaptivateException("Failed to initialize the hooks");


    running = true;
}

void KaptivateAPI::stopCapture()
{
    if(!running)
        throw KaptivateException("Kaptivate is not running");

    if(0 != kaptivateHookUninit())
        throw KaptivateException("Failed to uninitialize the hooks");

    // TODO: Send some kind of "stop" message to the main event loop

    // Wait for it to return
    WaitForSingleObject(msgLoopThread, INFINITE);

    running = false;
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
