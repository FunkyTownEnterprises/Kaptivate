/*
 * hooks.cpp
 * This file is a part of Kaptivate
 * https://github.com/FunkyTownEnterprises/Kaptivate
 *
 * Copyright (c) 2010 Ben Cable, Chris Eberle
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
#include "hooks.h"

#include <windows.h>
#include <stdio.h>

// The functions in this file are ultimately responsible for determining wheter or not
// other apps receive a particular mouse or keyboard event.

// <magic>
#pragma data_seg (".SHAREDMEMORY")

// The Win32 API actually loads this DLL into its own memory space. The only way to
// communicate is with a shared memory segment. Note that you can't share things on
// the heap (i.e. pointers are useless).

// Because the functions below are executed in a seperate memory space, calling off
// to other functions within this library can only be accomplished either with sockets
// or windows messaging. I'll let you guess which one we went with.

static short _initialized    = 0; // Are we ready to handle things?
static short _paused         = 0; // In paused mode, hook messages are passed along as normal
static HHOOK _keyboardHook   = 0; // A handle for the keyboard hook
static HHOOK _mouseHook      = 0; // Ditto for the mouse hook
static HWND  _callbackHwnd   = 0; // The Win32 window we'll be asking for decisions
static short _kbHookAlive    = 0; // A failsafe of sorts for the keyboard
static short _mouseHookAlive = 0; // Another failsafe for the mouse
static UINT  _keyboardMsg    = 0; // The custom keyboard message (generated with RegisterWindowMessage)
static UINT  _mouseMsg       = 0; // The custom mouse message
static UINT  _msgTimeout     = 0; // How long to wait before declaring it defunct (in milliseconds)

#pragma data_seg()
#pragma comment(linker,"/SECTION:.SHAREDMEMORY,RWS")
// </magic>

extern HMODULE kaptivateDllModule;

// Process a keyboard event. We can choose to pass the message along, or consume it.
LRESULT CALLBACK keyboardEvent(int nCode, WPARAM wParam, LPARAM lParam)
{
    // Should we even try?
    if(nCode < 0 || _paused == 1 || _callbackHwnd == 0 || _kbHookAlive == 0 || _initialized == 0 || _keyboardMsg == 0)
    {
        return CallNextHookEx(_keyboardHook, nCode, wParam, lParam);
    }

    LRESULT res;
    DWORD dwres = 0;

    // Wha'cha wanna do?
    if(0 == (res = SendMessageTimeout(_callbackHwnd, _keyboardMsg, wParam, lParam, SMTO_ABORTIFHUNG, _msgTimeout, &dwres)))
    {
        if(ERROR_TIMEOUT == GetLastError())
        {
            _kbHookAlive = 0;
            return CallNextHookEx(_keyboardHook, nCode, wParam, lParam);
        }
    }

    // > 0 means consume the keystroke
    if(dwres > 0)
    {
        // If the hook procedure processed the message, it may return a nonzero value
        // to prevent the system from passing the message to the rest of the hook chain
        // or the target window procedure.
        return 1;
    }

    // Business as usual
    return CallNextHookEx(_keyboardHook, nCode, wParam, lParam);
}

// Process a mouse event. Again, we can either pass it along or consume it.
LRESULT CALLBACK mouseEvent(int nCode, WPARAM wParam, LPARAM lParam)
{
    // Derp
    if(nCode < 0 || _paused == 1 || _callbackHwnd == 0 || _mouseHookAlive == 0 || _initialized == 0 || _mouseMsg == 0)
    {
        return CallNextHookEx(_mouseHook, nCode, wParam, lParam);
    }

    LRESULT res;
    DWORD dwres = 0;

    // Wha'cha wanna do?
    if(0 == (res = SendMessageTimeout(_callbackHwnd, _mouseMsg, wParam, lParam, SMTO_ABORTIFHUNG, _msgTimeout, &dwres)))
    {
        if(ERROR_TIMEOUT == GetLastError())
        {
            _mouseHookAlive = 0;
            return CallNextHookEx(_mouseHook, nCode, wParam, lParam);
        }
    }

    // > 0 means consume the mouse event
    if(dwres > 0)
    {
        // If the hook procedure processed the message, it may return a nonzero value
        // to prevent the system from passing the message to the rest of the hook chain
        // or the target window procedure.
        return 1;
    }

    // Business as usual
    return CallNextHookEx(_keyboardHook, nCode, wParam, lParam);
}

// Set up the hooks
int hookInit(HWND callbackWindow, UINT keyboardMessage, UINT mouseMessage, UINT messageTimeout)
{
    if(_initialized)
        return 0;

    // Who are we talking to (and how)?
    _callbackHwnd = callbackWindow;
    _keyboardMsg  = keyboardMessage;
    _mouseMsg     = mouseMessage;
    _msgTimeout   = messageTimeout;    

    // Set up a global keyboard hook
    if(0 == (_keyboardHook = SetWindowsHookEx(WH_KEYBOARD, keyboardEvent, kaptivateDllModule, 0)))
        return -1;

    // Set up a global mouse hook
    if(0 == (_mouseHook = SetWindowsHookEx(WH_MOUSE, mouseEvent, kaptivateDllModule, 0)))
        return -2;

    // We're ready to start processing keyboard / mouse events
    _initialized    = 1;
    _kbHookAlive    = 1;
    _mouseHookAlive = 1;

    return 0;
}

// Remove the hooks, clean up
int hookUninit()
{
    if(!_initialized)
        return 0;

    _kbHookAlive    = 0;
    _mouseHookAlive = 0;

    if(_mouseHook)
    {
        UnhookWindowsHookEx(_mouseHook);
        _mouseHook = 0;
    }

    if(_keyboardHook)
    {
        UnhookWindowsHookEx(_keyboardHook);
        _keyboardHook = 0;
    }

    _callbackHwnd = 0;
    _keyboardMsg  = 0;
    _mouseMsg     = 0;
    _msgTimeout   = 0;
    _initialized  = 0;

    return 0;
}

// Pause all processing of hooks
int hookPause()
{
    if(_initialized)
    {
        _paused = 1;
        return 0;
    }

    return -1;
}

// Resume all processing of hooks
int hookUnpause()
{
    if(_initialized)
    {
        _paused = 0;
        return 0;
    }

    return -1;
}
