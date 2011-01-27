/*
 * Kaptivate.h
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

#ifdef KAPTIVATE_EXPORTS
#define KAPTIVATE_API __declspec(dllexport)
#else
#define KAPTIVATE_API __declspec(dllimport)
#endif

#include <iostream>
#include <string>
#include <vector>

namespace Kaptivate
{
    struct KeyboardInfo
    {
        std::string id;
    };

    class KAPTIVATE_API KeyboardEvent
    {
    public:
        KeyboardEvent();
        ~KeyboardEvent();
    };

    class KAPTIVATE_API KeyboardHandler
    {
    public:
        virtual void HandleKeyEvent(KeyboardEvent& evt) = 0;
    };

    struct MouseInfo
    {
        std::string id;
    };

    class KAPTIVATE_API MouseButtonEvent
    {
    public:
        MouseButtonEvent();
        ~MouseButtonEvent();
    };

    class KAPTIVATE_API MouseWheelEvent
    {
    public:
        MouseWheelEvent();
        ~MouseWheelEvent();
    };

    class KAPTIVATE_API MouseMoveEvent
    {
    public:
        MouseMoveEvent();
        ~MouseMoveEvent();
    };

    class KAPTIVATE_API MouseHandler
    {
    public:
        virtual void HandleButtonEvent(MouseButtonEvent& evt) = 0;
        virtual void HandleWheelEvent(MouseWheelEvent& evt) = 0;
        virtual void HandleMoveEvent(MouseMoveEvent& evt) = 0;
    };

    // The main Kaptivate API
    class KAPTIVATE_API KaptivateAPI
    {
    private:

        // Private singleton methods
        KaptivateAPI();
        static bool instanceFlag;
        static KaptivateAPI *singleton;

        // Status
        bool running;
        bool suspended;

        // Stuff for the main message loop thread
        HANDLE msgLoopThread;
        HWND callbackWindow;

    public:

        // Public singleton methods
        ~KaptivateAPI();
        static KaptivateAPI* getInstance();
        static void destroyInstance();

        ////////////////////////////////////////////////////////////////////////////////
        // Kaptivate API methods

        // Start / stop
        void startCapture(bool wantMouse = true, bool wantKeyboard = true, bool startSuspended = false, UINT msgTimeoutMs = 5000);
        void stopCapture();

        // Suspend / resume
        void suspendCapture();
        void resumeCapture();

        // Status
        bool isRunning() const;
        bool isSuspended() const;

        // Enumeration
        std::vector<KeyboardInfo> enumerateKeyboards();
        std::vector<MouseInfo> enumerateMice();

        // Registration
        void registerKeyboardHandler(std::string idRegex, KeyboardHandler* handler);
        void resgisterMouseHandler(std::string idRegex, MouseHandler* handler);
        void unregisterKeyboardHandler(KeyboardHandler* handler);
        void unregisterMouseHandler(MouseHandler* handler);
    };
}
