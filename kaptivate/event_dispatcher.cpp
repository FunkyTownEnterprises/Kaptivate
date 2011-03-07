/*
 * event_dispatcher.cpp
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
#include "event_dispatcher.h"
#include "kaptivate.h"

#include <iostream>
using namespace std;
using namespace Kaptivate;

EventDispatcher::EventDispatcher()
{
}

EventDispatcher::~EventDispatcher()
{
}

void EventDispatcher::handleKeyboard(KeyboardEvent& evt)
{
}

void EventDispatcher::handleMouseButton(MouseButtonEvent& evt)
{
}

void EventDispatcher::handleMouseWheel(MouseWheelEvent& evt)
{
}

void EventDispatcher::handleMouseMove(MouseMoveEvent& evt)
{
}

vector<KeyboardInfo> EventDispatcher::enumerateKeyboards()
{
    vector<KeyboardInfo> dummy;
    return dummy;
}

vector<MouseInfo> EventDispatcher::enumerateMice()
{
    vector<MouseInfo> dummy;
    return dummy;
}

void EventDispatcher::registerKeyboardHandler(string idRegex, KeyboardHandler* handler)
{
}

void EventDispatcher::resgisterMouseHandler(string idRegex, MouseHandler* handler)
{
}

void EventDispatcher::unregisterKeyboardHandler(KeyboardHandler* handler)
{
}

void EventDispatcher::unregisterMouseHandler(MouseHandler* handler)
{
}

void EventDispatcher::scanDevices()
{
    UINT nDevices = 0;
    PRAWINPUTDEVICELIST pRawInputDeviceList = NULL;

    if(GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
    {
        // TODO: Throw an error
    }

    if(nDevices > 0)
    {
        if((pRawInputDeviceList = (PRAWINPUTDEVICELIST)malloc(sizeof(RAWINPUTDEVICELIST) * nDevices)) == NULL)
        {
            // TODO: Throw an error
        }

        if(GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1)
        {
            free(pRawInputDeviceList);
            // TODO: Throw an error
        }
    }

    // TODO: Lock

    {
        map<HANDLE, KeyboardInfo*>::iterator it;
        for(it = keyboardDevices.begin() ; it != keyboardDevices.end(); it++)
            delete (*it).second;
        keyboardDevices.clear();
    }

    {
        map<HANDLE, MouseInfo*>::iterator it;
        for(it = mouseDevices.begin() ; it != mouseDevices.end(); it++)
            delete (*it).second;
        mouseDevices.clear();
    }

    for(UINT i = 0; i < nDevices; i++)
    {
        RAWINPUTDEVICELIST& rid = pRawInputDeviceList[i];
        if(rid.dwType != RIM_TYPEKEYBOARD && rid.dwType != RIM_TYPEMOUSE)
            continue;
        if(keyboardDevices.count(rid.hDevice) > 0)
            continue;
        if(mouseDevices.count(rid.hDevice) > 0)
            continue;

        UINT pcbSize = 0;
        if(0 != GetRawInputDeviceInfo(rid.hDevice, RIDI_DEVICENAME, NULL, &pcbSize))
            continue;

        if (pcbSize > 0)
        {
            char* cDevName = (char*)malloc(sizeof(TCHAR) * pcbSize);
            if(cDevName != NULL)
            {
                if(GetRawInputDeviceInfo(rid.hDevice, RIDI_DEVICENAME, (LPVOID)cDevName, &pcbSize) > 0)
                {
                    string devName(cDevName); 
                    if(rid.dwType == RIM_TYPEKEYBOARD)
                    {
                        KeyboardInfo* kbi = new KeyboardInfo();
                        kbi->device = rid.hDevice;
                        kbi->name = devName;
                        keyboardDevices[rid.hDevice] = kbi;
                    }
                    else if(rid.dwType == RIM_TYPEMOUSE)
                    {
                        MouseInfo* mi = new MouseInfo();
                        mi->device = rid.hDevice;
                        mi->name = devName;
                        mouseDevices[rid.hDevice] = mi;
                    }
                }

                free(cDevName);
            }
        }
    }

    // TODO: Unlock

    if(pRawInputDeviceList)
        free(pRawInputDeviceList);
}
