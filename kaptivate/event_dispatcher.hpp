/*
 * event_dispatcher.h
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

#pragma once

#include <map>
#include <vector>
#include <iostream>

class TRexpp;

namespace Kaptivate
{
    class KeyboardEvent;
    class MouseButtonEvent;
    class MouseWheelEvent;
    class MouseMoveEvent;
    class KeyboardHandler;
    class MouseHandler;
    class KeyboardEventChain;
    class MouseEventChain;
    struct KeyboardInfo;
    struct MouseInfo;

    struct RexHandler
    {
        TRexpp* rex;
        union
        {
            KeyboardHandler* khandler;
            MouseHandler* mhandler;
        };
    };

    class EventDispatcher
    {
    private:
        HANDLE kdLock;
        HANDLE mdLock;
        std::map<HANDLE, KeyboardInfo*> keyboardDevices;
        std::map<HANDLE, MouseInfo*> mouseDevices;

        HANDLE kecLock;
        HANDLE mecLock;
        std::map<HANDLE, KeyboardEventChain*> kbdEventChains;
        std::map<HANDLE, MouseEventChain*> mouseEventChains;

        HANDLE kbHRMLock;
        HANDLE mdHRMLock;

        std::multimap<std::string, RexHandler*> kHandlerRexMap;
        std::multimap<std::string, RexHandler*> mHandlerRexMap;

        RexHandler* getKeyboardHandler(std::string regex, KeyboardHandler* handler);
        RexHandler* getMouseHandler(std::string regex, MouseHandler* handler);

        KeyboardInfo* unknownKeyboardDevice(HANDLE device);
        MouseInfo* unknownMouseDevice(HANDLE device);

        void newKeyboardDevice(KeyboardInfo* info);
        void newMouseDevice(MouseInfo* info);
        void newKeyboardHandler(RexHandler* keHandler);
        void newMouseHandler(RexHandler* meHandler);

        void cleanupMouseHandlerMap();
        void cleanupKeyboardHandlerMap();
        void cleanupMouseEventChainMap();
        void cleanupKeyboardEventChainMap();
        void cleanupMouseDeviceMap();
        void cleanupKeyboardDeviceMap();

        void scanDevices();

    public:
        EventDispatcher();
        ~EventDispatcher();

        void handleKeyboard(KeyboardEvent& evt);
        void handleMouseButton(MouseButtonEvent& evt);
        void handleMouseWheel(MouseWheelEvent& evt);
        void handleMouseMove(MouseMoveEvent& evt);

        std::vector<KeyboardInfo> enumerateKeyboards();
        std::vector<MouseInfo> enumerateMice();

        void registerKeyboardHandler(std::string idRegex, KeyboardHandler* handler);
        void resgisterMouseHandler(std::string idRegex, MouseHandler* handler);
        void unregisterKeyboardHandler(KeyboardHandler* handler);
        void unregisterMouseHandler(MouseHandler* handler);
    };
}
