/*
 * kaptivate.h
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
    // What should be done with a particular event?
    enum Decision
    {
        UNDECIDED = 1,
        PERMIT = 2,
        CONSUME = 4,
        PASS = 8
    };

    // Information about a particular keyboard
    struct KeyboardInfo
    {
        HANDLE device;
        std::string name;
    };

    // Describes a keyboard event
    class KAPTIVATE_API KeyboardEvent
    {
    private:
        HANDLE device;
        unsigned int vkey;
        unsigned int scanCode;
        unsigned int wmMessage;
        bool keyUp;
        Decision decision;

    public:
        KeyboardEvent(HANDLE device, unsigned int vkey, unsigned int scanCode, unsigned int wmMessage, bool keyUp);
        ~KeyboardEvent();

        HANDLE getDeviceHandle() const;
        unsigned int getVkey() const;
        unsigned int getScanCode() const;
        unsigned int getWindowMessage() const;
        bool getKeyUp() const;

        Decision getDecision() const;
        void setDecision(Decision decision);
    };

    // An interface for a keyboard event handler
    class KAPTIVATE_API KeyboardHandler
    {
    public:
        virtual void HandleKeyEvent(KeyboardEvent& evt) = 0;
    };

    // Information about a particular mouse
    struct MouseInfo
    {
        HANDLE device;
        std::string name;
    };

    // Describes a mouse button event
    class KAPTIVATE_API MouseButtonEvent
    {
    private:
        Decision decision;

    public:
        MouseButtonEvent();
        ~MouseButtonEvent();

        Decision getDecision() const;
        void setDecision(Decision decision);
    };

    // Describes a mouse wheel event
    class KAPTIVATE_API MouseWheelEvent
    {
    private:
        Decision decision;

    public:
        MouseWheelEvent();
        ~MouseWheelEvent();

        Decision getDecision() const;
        void setDecision(Decision decision);
    };

    // Describes a mouse move event
    class KAPTIVATE_API MouseMoveEvent
    {
    private:
        Decision decision;

    public:
        MouseMoveEvent();
        ~MouseMoveEvent();

        Decision getDecision() const;
        void setDecision(Decision decision);
    };

    // An interface for a mouse event handler
    class KAPTIVATE_API MouseHandler
    {
    public:
        virtual void HandleButtonEvent(MouseButtonEvent& evt) = 0;
        virtual void HandleWheelEvent(MouseWheelEvent& evt) = 0;
        virtual void HandleMoveEvent(MouseMoveEvent& evt) = 0;
    };

    // Dummy declarations
    class EventDispatcher;
    class EventQueue;

    // The main Kaptivate API
    class KAPTIVATE_API KaptivateAPI
    {
    private:

        // Private singleton methods
        KaptivateAPI();
        static bool instanceFlag;
        static KaptivateAPI *singleton;

        // Class members
        EventDispatcher* dispatcher;
        EventQueue* events;

        // Status
        bool running;
        bool suspended;

        // Stuff for the main message loop thread
        HANDLE msgLoopThread;
        HWND callbackWindow;

        // To keep track of what type of devices we want from the raw API
        bool rawKeyboardRunning;
        bool rawMouseRunning;
        bool userWantsMouse;
        bool userWantsKeyboard;

        // Internal utility methods
        bool tryStopMsgLoop();
        bool pingMessageWindow() const;
        bool startRawCapture(bool wantMouse, bool wantKeyboard);
        bool stopRawCapture();

        // Message processing methods
        void ProcessRawInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

        LRESULT ProcessMouseHook(HWND hWnd, WPARAM wParam, LPARAM lParam);
        void ProcessRawMouseInput(RAWINPUT* raw);

        LRESULT ProcessKeyboardHook(HWND hWnd, WPARAM wParam, LPARAM lParam);
        void ProcessRawKeyboardInput(RAWINPUT* raw);

        // Message processing members
        UINT keyboardMessage;
        UINT mouseMessage;
        UINT pingMessage;

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

        // Window message processing
        LRESULT _ProcessWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    };
}
