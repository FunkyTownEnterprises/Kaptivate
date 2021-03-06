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

#include "stdafx.hpp"
#include "event_dispatcher.hpp"
#include "kaptivate.hpp"
#include "kaptivate_exceptions.hpp"
#include "scoped_mutex.hpp"
#include "event_chain.hpp"

#include "trex/trex.hpp"
#include "trex/TRexpp.hpp"

#include <iostream>
using namespace std;
using namespace Kaptivate;

// Constructor
EventDispatcher::EventDispatcher()
{
    InitializeCriticalSection(&kdLock);
    InitializeCriticalSection(&mdLock);
    InitializeCriticalSection(&kbHRMLock);
    InitializeCriticalSection(&mdHRMLock);
    InitializeCriticalSection(&kecLock);
    InitializeCriticalSection(&mecLock);
}

// Destructor
EventDispatcher::~EventDispatcher()
{
    cleanupMouseHandlerMap();
    cleanupKeyboardHandlerMap();
    cleanupMouseEventChainMap();
    cleanupKeyboardEventChainMap();
    cleanupMouseDeviceMap();
    cleanupKeyboardDeviceMap();

    DeleteCriticalSection(&kdLock);
    DeleteCriticalSection(&mdLock);
    DeleteCriticalSection(&kbHRMLock);
    DeleteCriticalSection(&mdHRMLock);
    DeleteCriticalSection(&kecLock);
    DeleteCriticalSection(&mecLock);
}

// Dispatch a keyboard event to any registered handlers
void EventDispatcher::handleKeyboard(KeyboardEvent& evt)
{
    HANDLE dev = evt.getDeviceHandle();

    {
        // Set the device info if it's available
        ScopedCriticalSection kMutex(&kdLock);
        if(keyboardDevices.count(dev) > 0)
        {
            evt.setDeviceInfo(keyboardDevices[dev]);
        }
        else
        {
            // It's an unknown device, let's see what we can find out about it.
            ScopedNonCriticalSection unlock(&kdLock);
            KeyboardInfo *kbd = unknownKeyboardDevice(dev);
            if(kbd)
            {
                evt.setDeviceInfo(kbd);
            }
        }
    }

    {
        // Run the chain, let someone make a decision about this event
        ScopedCriticalSection kecMutex(&kecLock);
        if(kbdEventChains.count(dev) > 0 && kbdEventChains[dev]->chainSize() > 0)
        {
            // We've got a real handler, give it to them (and hard)
            kbdEventChains[dev]->runKeyboardEventChain(evt);
        }
        else
        {
            // Ain't nobody here. Do something?
        }
    }

}

// Dispatch a mouse button event to any registered handlers
void EventDispatcher::handleMouseButton(MouseButtonEvent& evt)
{
    HANDLE dev = evt.getDeviceHandle();

    {
        // Set the device info if it's available
        ScopedCriticalSection mMutex(&mdLock);
        if(mouseDevices.count(dev) > 0)
        {
            evt.setDeviceInfo(mouseDevices[dev]);
        }
        else
        {
            // It's an unknown device, let's see what we can find out about it.
            ScopedNonCriticalSection unlock(&mdLock);
            MouseInfo *mi = unknownMouseDevice(dev);
            if(mi)
            {
                evt.setDeviceInfo(mi);
            }
        }
    }

    {
        // Run the chain, let someone make a decision about this event
        ScopedCriticalSection mecMutex(&mecLock);
        if(mouseEventChains.count(dev) > 0 && mouseEventChains[dev]->chainSize() > 0)
        {
            // You like me! You really, really like me!
            mouseEventChains[dev]->runMouseButtonEventChain(evt);
        }
        else
        {
            // Zero is the REAL lonliest number. It's the lonliest number
            // since the number one.
        }
    }
}

// Dispatch a mouse wheel event to any registered handlers
void EventDispatcher::handleMouseWheel(MouseWheelEvent& evt)
{
    HANDLE dev = evt.getDeviceHandle();

    {
        // Set the device name if it's available
        ScopedCriticalSection mMutex(&mdLock);
        if(mouseDevices.count(dev) > 0)
        {
            evt.setDeviceInfo(mouseDevices[dev]);
        }
        else
        {
            // It's an unknown device, let's see what we can find out about it.
            ScopedNonCriticalSection unlock(&mdLock);
            MouseInfo *mi = unknownMouseDevice(dev);
            if(mi)
            {
                evt.setDeviceInfo(mi);
            }
        }
    }

    {
        // Run the chain, let someone make a decision about this event
        ScopedCriticalSection mecMutex(&mecLock);
        if(mouseEventChains.count(dev) > 0 && mouseEventChains[dev]->chainSize() > 0)
        {
            // You like me! You really, really like me!
            mouseEventChains[dev]->runMouseWheelEventChain(evt);
        }
        else
        {
            // Zero is the REAL lonliest number. It's the lonliest number
            // since the number one.
        }
    }
}

// Dispatch a mouse move event to any registered handlers
void EventDispatcher::handleMouseMove(MouseMoveEvent& evt)
{
    HANDLE dev = evt.getDeviceHandle();

    {
        // Set the device name if it's available
        ScopedCriticalSection mMutex(&mdLock);
        if(mouseDevices.count(dev) > 0)
        {
            evt.setDeviceInfo(mouseDevices[dev]);
        }
        else
        {
            // It's an unknown device, let's see what we can find out about it.
            ScopedNonCriticalSection unlock(&mdLock);
            MouseInfo *mi = unknownMouseDevice(dev);
            if(mi)
            {
                evt.setDeviceInfo(mi);
            }
        }
    }

    {
        // Run the chain, let someone make a decision about this event
        ScopedCriticalSection mecMutex(&mecLock);
        if(mouseEventChains.count(dev) > 0 && mouseEventChains[dev]->chainSize() > 0)
        {
            // You like me! You really, really like me!
            mouseEventChains[dev]->runMouseMoveEventChain(evt);
        }
        else
        {
            // Zero is the REAL lonliest number. It's the lonliest number
            // since the number one.
        }
    }
}

// Get a list of attached keyboards
vector<KeyboardInfo> EventDispatcher::enumerateKeyboards()
{
    vector<KeyboardInfo> ret;
    scanDevices();

    {
        ScopedCriticalSection kMutex(&kdLock);

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
        ScopedCriticalSection mMutex(&mdLock);

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
    RexHandler* rex = getKeyboardHandler(idRegex, handler);
    newKeyboardHandler(rex);
}

// Register a handler for mouse events
void EventDispatcher::resgisterMouseHandler(string idRegex, MouseHandler* handler)
{
    RexHandler* rex = getMouseHandler(idRegex, handler);
    newMouseHandler(rex);
}

// Unregister a handler for keyboard events
void EventDispatcher::unregisterKeyboardHandler(KeyboardHandler* handler)
{
    ScopedCriticalSection kecMutex(&kecLock);
    map<HANDLE, KeyboardEventChain*>::iterator it;
    for(it = kbdEventChains.begin(); it != kbdEventChains.end(); it++)
        (*it).second->removeHandler(handler);
}

// Unregister a handler for mouse events
void EventDispatcher::unregisterMouseHandler(MouseHandler* handler)
{
    ScopedCriticalSection mecMutex(&mecLock);
    map<HANDLE, MouseEventChain*>::iterator it;
    for(it = mouseEventChains.begin(); it != mouseEventChains.end(); it++)
        (*it).second->removeHandler(handler);
}

// Add or get a mouse handler for a given regular expression and handler pair
RexHandler* EventDispatcher::getMouseHandler(std::string regex, MouseHandler* handler)
{
    ScopedCriticalSection mMutex(&mdHRMLock);

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
RexHandler* EventDispatcher::getKeyboardHandler(string regex, KeyboardHandler* handler)
{
    ScopedCriticalSection kMutex(&kbHRMLock);

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
    ScopedCriticalSection mMutex(&mdHRMLock);
    multimap<string, RexHandler*>::iterator it;

    for(it = mHandlerRexMap.begin(); it != mHandlerRexMap.end(); it++)
    {
        RexHandler* rex = (*it).second;
        delete rex->rex;
        delete rex;
    }

    mHandlerRexMap.clear();
}

// Clean up the mouse handler map
void EventDispatcher::cleanupKeyboardHandlerMap()
{
    ScopedCriticalSection kMutex(&kbHRMLock);
    multimap<string, RexHandler*>::iterator it;

    for(it = kHandlerRexMap.begin(); it != kHandlerRexMap.end(); it++)
    {
        RexHandler* rex = (*it).second;
        delete rex->rex;
        delete rex;
    }

    kHandlerRexMap.clear();
}

// Clean up the mouse event chain map
void EventDispatcher::cleanupMouseEventChainMap()
{
    ScopedCriticalSection mMutex(&mecLock);

    map<HANDLE, MouseEventChain*>::iterator it;
    for(it = mouseEventChains.begin(); it != mouseEventChains.end(); it++)
        delete (*it).second;
    mouseEventChains.clear();
}

// Clean up the keyboard event chain map
void EventDispatcher::cleanupKeyboardEventChainMap()
{
    ScopedCriticalSection kMutex(&kecLock);

    map<HANDLE, KeyboardEventChain*>::iterator it;
    for(it = kbdEventChains.begin(); it != kbdEventChains.end(); it++)
        delete (*it).second;
    kbdEventChains.clear();
}

// Clear out the mouse device info map
void EventDispatcher::cleanupMouseDeviceMap()
{
    ScopedCriticalSection mMutex(&mdLock);

    map<HANDLE, MouseInfo*>::iterator it;
    for(it = mouseDevices.begin(); it != mouseDevices.end(); it++)
        delete (*it).second;
    mouseDevices.clear();
}

// Clear out the keyboard device info map
void EventDispatcher::cleanupKeyboardDeviceMap()
{
    ScopedCriticalSection kMutex(&kdLock);

    map<HANDLE, KeyboardInfo*>::iterator it;
    for(it = keyboardDevices.begin() ; it != keyboardDevices.end(); it++)
        delete (*it).second;
    keyboardDevices.clear();
}

// A new mouse device has been added
void EventDispatcher::newMouseDevice(MouseInfo* info)
{
    ScopedCriticalSection mMutex(&mdHRMLock);
    multimap<string, RexHandler*>::iterator it;

    for(it = mHandlerRexMap.begin(); it != mHandlerRexMap.end(); it++)
    {
        RexHandler* rh = (*it).second;
        if(rh->rex->Match(info->name.c_str()))
        {
            // OK, we've got a registered handler for this device.
            ScopedCriticalSection mecMutex(&mecLock);
            if(mouseEventChains.count(info->device) == 0)
                mouseEventChains[info->device] = new MouseEventChain();
            mouseEventChains[info->device]->addHandler(rh->mhandler);
        }
    }
}

// A new keyboard device has been added
void EventDispatcher::newKeyboardDevice(KeyboardInfo* info)
{
    ScopedCriticalSection kMutex(&kbHRMLock);
    multimap<string, RexHandler*>::iterator it;

    for(it = kHandlerRexMap.begin(); it != kHandlerRexMap.end(); it++)
    {
        RexHandler* rh = (*it).second;
        if(rh->rex->Match(info->name.c_str()))
        {
            // OK, we've got a registered handler for this device.
            ScopedCriticalSection kecMutex(&kecLock);
            if(kbdEventChains.count(info->device) == 0)
                kbdEventChains[info->device] = new KeyboardEventChain();
            kbdEventChains[info->device]->addHandler(rh->khandler);
        }
    }
}

// A new keyboard event handler has been added
void EventDispatcher::newKeyboardHandler(RexHandler* keHandler)
{
    ScopedCriticalSection kMutex(&kdLock);

    map<HANDLE, KeyboardInfo*>::iterator it;
    for(it = keyboardDevices.begin(); it != keyboardDevices.end(); it++)
    {
        KeyboardInfo* info = (*it).second;
        if(keHandler->rex->Match(info->name.c_str()))
        {
            // OK, our new handler can handle this device
            ScopedCriticalSection kecMutex(&kecLock);
            if(kbdEventChains.count(info->device) == 0)
                kbdEventChains[info->device] = new KeyboardEventChain();
            kbdEventChains[info->device]->addHandler(keHandler->khandler);
        }
    }
}

// A new mouse event handler has been added
void EventDispatcher::newMouseHandler(RexHandler* meHandler)
{
    ScopedCriticalSection mMutex(&mdHRMLock);

    map<HANDLE, MouseInfo*>::iterator it;
    for(it = mouseDevices.begin(); it != mouseDevices.end(); it++)
    {
        MouseInfo* info = (*it).second;
        if(meHandler->rex->Match(info->name.c_str()))
        {
            // OK, our new handler can handle this device
            ScopedCriticalSection mecMutex(&mecLock);
            if(mouseEventChains.count(info->device) == 0)
                mouseEventChains[info->device] = new MouseEventChain();
            mouseEventChains[info->device]->addHandler(meHandler->mhandler);
        }
    }
}

// An unknown keyboard device has been encountered. Find out more about it.
KeyboardInfo* EventDispatcher::unknownKeyboardDevice(HANDLE device)
{
    // TODO: Find out more about this new device

    return NULL;
}

// An unknown mouse device has been encountered. Find out more about it.
MouseInfo* EventDispatcher::unknownMouseDevice(HANDLE device)
{
    // TODO: Find out more about this new device

    return NULL;
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

    // We'll be rebuilding these
    cleanupKeyboardEventChainMap();
    cleanupMouseEventChainMap();
    cleanupKeyboardDeviceMap();
    cleanupMouseDeviceMap();

    {
        // Begin lock

        ScopedCriticalSection kMutex(&kdLock);
        ScopedCriticalSection mMutex(&mdLock);

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
                TCHAR* cDevName = (TCHAR*)malloc(sizeof(TCHAR) * pcbSize);
                if(cDevName != NULL)
                {
                    // Get the device name
                    if(GetRawInputDeviceInfo(rid.hDevice, RIDI_DEVICENAME, (LPVOID)cDevName, &pcbSize) > 0)
                    {
#ifdef UNICODE
                        wstring wdevName(cDevName);
                        string devName(wdevName.begin(), wdevName.end());
                        devName.assign(wdevName.begin(), wdevName.end());
#else
                        string devName(cDevName); 
#endif
                        
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
