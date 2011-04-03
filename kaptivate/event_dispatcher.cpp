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
#include "kaptivate_exceptions.h"
#include "scoped_mutex.h"
#include "event_chain.h"

#include "trex/trex.h"
#include "trex/TRexpp.h"

#include <iostream>
using namespace std;
using namespace Kaptivate;

// Constructor
EventDispatcher::EventDispatcher()
{
    kdLock = CreateMutex(NULL, FALSE, NULL);
    mdLock = CreateMutex(NULL, FALSE, NULL);
    kbHRMLock = CreateMutex(NULL, FALSE, NULL);
    mdHRMLock = CreateMutex(NULL, FALSE, NULL);
}

// Destructor
EventDispatcher::~EventDispatcher()
{
    CloseHandle(kdLock);
    CloseHandle(mdLock);
    CloseHandle(kbHRMLock);
    CloseHandle(mdHRMLock);

    cleanupMouseHandlerMap();
    cleanupKeyboardHandlerMap();
}

// Dispatch a keyboard event to any registered handlers
void EventDispatcher::handleKeyboard(KeyboardEvent& evt)
{

}

// Dispatch a mouse button event to any registered handlers
void EventDispatcher::handleMouseButton(MouseButtonEvent& evt)
{
}

// Dispatch a mouse wheel event to any registered handlers
void EventDispatcher::handleMouseWheel(MouseWheelEvent& evt)
{
}

// Dispatch a mouse move event to any registered handlers
void EventDispatcher::handleMouseMove(MouseMoveEvent& evt)
{
}

// Get a list of attached keyboards
vector<KeyboardInfo> EventDispatcher::enumerateKeyboards()
{
    vector<KeyboardInfo> ret;
    scanDevices();

    {
        ScopedLock kMutex(kdLock);

        map<HANDLE, KeyboardInfo*>::iterator it;
        for(it = keyboardDevices.begin() ; it != keyboardDevices.end(); it++)
        {
            KeyboardInfo inf;
            inf.device = (*it).second->device;
            inf.name = (*it).second->name;
            ret.push_back(inf);
        }
    }

    return ret;
}

// Get a list of attached mice
vector<MouseInfo> EventDispatcher::enumerateMice()
{
    vector<MouseInfo> ret;
    scanDevices();

    {
        ScopedLock mMutex(mdLock);

        map<HANDLE, MouseInfo*>::iterator it;
        for(it = mouseDevices.begin() ; it != mouseDevices.end(); it++)
        {
            MouseInfo inf;
            inf.device = (*it).second->device;
            inf.name = (*it).second->name;
            ret.push_back(inf);
        }
    }

    return ret;
}

// Register a handler for keyboard events
void EventDispatcher::registerKeyboardHandler(string idRegex, KeyboardHandler* handler)
{
    //addKeyboardHandler(idRegex, handler);
}

// Register a handler for mouse events
void EventDispatcher::resgisterMouseHandler(string idRegex, MouseHandler* handler)
{
    //addMouseHandler(idRegex, handler);
}

// Unregister a handler for keyboard events
void EventDispatcher::unregisterKeyboardHandler(KeyboardHandler* handler)
{
}

// Unregister a handler for mouse events
void EventDispatcher::unregisterMouseHandler(MouseHandler* handler)
{
}

// Add or get a mouse handler for a given regular expression and handler pair
RexHandler* EventDispatcher::addMouseHandler(std::string regex, MouseHandler* handler)
{
    ScopedLock mMutex(mdHRMLock);

    multimap<string, RexHandler*>::iterator it;
    pair<multimap<string, RexHandler*>::iterator, multimap<string, RexHandler*>::iterator> ret;
    ret = mHandlerRexMap.equal_range(regex);

    // Check to see if we've already registered this handler for this regex
    for (it = ret.first; it != ret.second; ++it)
    {
        if((*it).second->mhandler == handler)
            return (*it).second;
    }

    // No? Alright then register everything
    RexHandler* rh = new RexHandler();
    rh->mhandler = handler;
    rh->rex = new TRexpp();
    rh->rex->Compile(regex.c_str());

    mHandlerRexMap.insert(pair<string, RexHandler*>(regex, rh));
    return rh;
}

// Add or get a keyboard handler for a given regular expression and handler pair
RexHandler* EventDispatcher::addKeyboardHandler(string regex, KeyboardHandler* handler)
{
    ScopedLock kMutex(kbHRMLock);

    multimap<string, RexHandler*>::iterator it;
    pair<multimap<string, RexHandler*>::iterator, multimap<string, RexHandler*>::iterator> ret;
    ret = kHandlerRexMap.equal_range(regex);

    // Check to see if we've already registered this handler for this regex
    for (it = ret.first; it != ret.second; ++it)
    {
        if((*it).second->khandler == handler)
            return (*it).second;
    }

    // No? Alright then register everything
    RexHandler* rh = new RexHandler();
    rh->khandler = handler;
    rh->rex = new TRexpp();
    rh->rex->Compile(regex.c_str());

    kHandlerRexMap.insert(pair<string, RexHandler*>(regex, rh));
    return rh;
}

// Clean up the mouse handler map
void EventDispatcher::cleanupMouseHandlerMap()
{
    ScopedLock mMutex(mdHRMLock);
    multimap<string, RexHandler*>::iterator it;

    for(it = mHandlerRexMap.begin(); it != mHandlerRexMap.end(); it++)
    {
        RexHandler* rex = (*it).second;
        delete rex->rex;
        delete rex;

        mHandlerRexMap.erase(it);
    }
}

// Clean up the mouse handler map
void EventDispatcher::cleanupKeyboardHandlerMap()
{
    ScopedLock kMutex(kbHRMLock);
    multimap<string, RexHandler*>::iterator it;

    for(it = kHandlerRexMap.begin(); it != kHandlerRexMap.end(); it++)
    {
        RexHandler* rex = (*it).second;
        delete rex->rex;
        delete rex;

        kHandlerRexMap.erase(it);
    }
}

// A new mouse device has been added
void EventDispatcher::newMouseDevice(MouseInfo* info)
{
    ScopedLock mMutex(mdHRMLock);
    multimap<string, RexHandler*>::iterator it;

    for(it = mHandlerRexMap.begin(); it != mHandlerRexMap.end(); it++)
    {
        RexHandler* rh = (*it).second;
        if(rh->rex->Match(info->name.c_str()))
        {
            // OK, we've got a registered handler for this device.
            // Update the map (i.e. handlerMap[info->device] = rh->mhandler);
        }
    }
}

// A new keyboard device has been added
void EventDispatcher::newKeyboardDevice(KeyboardInfo* info)
{
    ScopedLock kMutex(kbHRMLock);
    multimap<string, RexHandler*>::iterator it;

    for(it = kHandlerRexMap.begin(); it != kHandlerRexMap.end(); it++)
    {
        RexHandler* rh = (*it).second;
        if(rh->rex->Match(info->name.c_str()))
        {
            // OK, we've got a registered handler for this device.
            // Update the map (i.e. handlerMap[info->device] = rh->khandler);
            if(kbdEventChains.count(info->device) == 0)
                kbdEventChains[info->device] = new KeyboardEventChain();
            kbdEventChains[info->device]->addHandler(rh->khandler);
        }
    }
}

// Scan the raw devices and fill in the device info structures
void EventDispatcher::scanDevices()
{
    UINT nDevices = 0;
    PRAWINPUTDEVICELIST pRawInputDeviceList = NULL;

    // Get the raw device count
    if(GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
        throw KaptivateException("Call to GetRawInputDeviceList(1) failed");

    if(nDevices > 0)
    {
        // Get the actual list of raw devices
        if((pRawInputDeviceList = (PRAWINPUTDEVICELIST)malloc(sizeof(RAWINPUTDEVICELIST) * nDevices)) == NULL)
            throw KaptivateException("Failed to allocate memory for the raw device list");
        if(GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1)
        {
            free(pRawInputDeviceList);
            throw KaptivateException("Call to GetRawInputDeviceList(2) failed");
        }
    }

    {
        // Begin lock

        ScopedLock kMutex(kdLock);
        ScopedLock mMutex(mdLock);

        {
            // Clear out the keyboardDevices map
            map<HANDLE, KeyboardInfo*>::iterator it;
            for(it = keyboardDevices.begin() ; it != keyboardDevices.end(); it++)
                delete (*it).second;
            keyboardDevices.clear();
        }

        {
            // Clear out the mouseDevices map
            map<HANDLE, MouseInfo*>::iterator it;
            for(it = mouseDevices.begin() ; it != mouseDevices.end(); it++)
                delete (*it).second;
            mouseDevices.clear();
        }

        // Iterate over the raw devices
        for(UINT i = 0; i < nDevices; i++)
        {
            RAWINPUTDEVICELIST& rid = pRawInputDeviceList[i];
            if(rid.dwType != RIM_TYPEKEYBOARD && rid.dwType != RIM_TYPEMOUSE)
                continue;
            if(keyboardDevices.count(rid.hDevice) > 0 || mouseDevices.count(rid.hDevice) > 0)
                continue;

            // Get the length of the device name
            UINT pcbSize = 0;
            if(0 != GetRawInputDeviceInfo(rid.hDevice, RIDI_DEVICENAME, NULL, &pcbSize))
                continue;

            if (pcbSize > 0)
            {
                char* cDevName = (char*)malloc(sizeof(TCHAR) * pcbSize);
                if(cDevName != NULL)
                {
                    // Get the device name
                    if(GetRawInputDeviceInfo(rid.hDevice, RIDI_DEVICENAME, (LPVOID)cDevName, &pcbSize) > 0)
                    {
                        string devName(cDevName); 
                        if(rid.dwType == RIM_TYPEKEYBOARD)
                        {
                            if(keyboardDevices.count(rid.hDevice) == 0)
                            {
                                // Process the new keyboard device
                                KeyboardInfo* kbi = new KeyboardInfo();
                                kbi->device = rid.hDevice;
                                kbi->name = devName;
                                keyboardDevices[rid.hDevice] = kbi;

                                newKeyboardDevice(kbi);
                            }
                        }
                        else if(rid.dwType == RIM_TYPEMOUSE)
                        {
                            if(mouseDevices.count(rid.hDevice))
                            {
                                // Process the new mouse device
                                MouseInfo* mi = new MouseInfo();
                                mi->device = rid.hDevice;
                                mi->name = devName;
                                mouseDevices[rid.hDevice] = mi;

                                newMouseDevice(mi);
                            }
                        }
                    }

                    free(cDevName);
                }
            }
        }

        // End lock
    }

    if(pRawInputDeviceList)
        free(pRawInputDeviceList);
}
