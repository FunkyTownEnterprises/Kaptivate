/*
 * sample_app.cpp
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
 * THIS SOFT
 */

#include "stdafx.hpp"

#include <vector>
#include <iostream>
using namespace std;

#include "kaptivate.hpp"
#include "kaptivate_exceptions.hpp"
using namespace Kaptivate;

class KeyPrinter : public KeyboardHandler
{
public:
    virtual void HandleKeyEvent(KeyboardEvent& evt)
    {
        // Show some info about the event
        cout << "Got a keyboard event:" << endl;
        cout << " device: " << evt.getDeviceInfo()->name << endl;
        cout << " vkey: " << evt.getVkey() << endl;
        cout << " scan code: " << evt.getScanCode() << endl;
        cout << " key up?: " << evt.getKeyUp() << endl;
        cout << endl;
    }
};

class SpaceEater : public KeyboardHandler
{
public:
    virtual void HandleKeyEvent(KeyboardEvent& evt)
    {
        // Decide whether or not to consume the keystroke
        if(evt.getVkey() == VK_SPACE)
        {
            evt.setDecision(CONSUME);
            cout << "Nom nom nom" << endl << endl;
        }
    }
};

class ReturnAllowifier : public KeyboardHandler
{
private:
    unsigned int counter;

public:

    ReturnAllowifier()
    {
        counter = 10;
    }

    virtual void HandleKeyEvent(KeyboardEvent& evt)
    {
        // Decide whether or not to consume the keystroke
        if(evt.getVkey() == VK_RETURN)
        {
            if(counter > 0)
            {
                counter--;
                evt.setDecision(PERMIT);
                cout << "I'll allow it - this time." << endl << endl;
            }
            else
            {
                evt.setDecision(CONSUME);
                cout << "No more return for you (press 'R' to reset)" << endl << endl;
            }
        }
        else if(evt.getVkey() == 82 && counter == 0)
        {
            if(evt.getKeyUp())
            {
                counter = 10;
                cout << "Reset the return count :)" << endl << endl;
            }

            evt.setDecision(CONSUME);
        }
    }
};

// Extend the keyboard handler. Must implement HandleKeyEvent
class EscapeHandler : public KeyboardHandler
{
private:
    HANDLE stopEvent;

public:
    EscapeHandler(HANDLE stopEvent)
    {
        this->stopEvent = stopEvent;
    }

    virtual void HandleKeyEvent(KeyboardEvent& evt)
    {
        if(evt.getVkey() == VK_ESCAPE)
        {
            evt.setDecision(CONSUME);
            cout << "Goodbye, cruel world" << endl;

            SetEvent(stopEvent);
        }
    }
};

int _tmain(int argc, _TCHAR* argv[])
{
    KaptivateAPI* kap = NULL;

    HANDLE stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    SpaceEater spaceHandler;
    KeyPrinter printHandler;
    ReturnAllowifier returnHandler;
    EscapeHandler escapeHandler(stopEvent);

    try
    {
        cout << "* Creating kaptivate instance... ";
        if(NULL == (kap = KaptivateAPI::getInstance()))
        {
            cout << "error." << endl;
            exit(1);
        }
        cout << "ok." << endl << endl;

        cout << "* List of keyboard devices: " << endl;
        vector<KeyboardInfo> keyboards = kap->enumerateKeyboards();
        vector<KeyboardInfo>::iterator kit;
        for(kit = keyboards.begin(); kit != keyboards.end(); kit++)
            cout << "  -> " << (*kit).name << endl;
        cout << endl;

        cout << "* Registering keyboard handlers... ";

        // NOTE: Order DOES matter. The first registered is the last called
        kap->registerKeyboardHandler(".*", &printHandler);
        kap->registerKeyboardHandler(".*", &spaceHandler);
        kap->registerKeyboardHandler(".*", &returnHandler);
        kap->registerKeyboardHandler(".*", &escapeHandler);

        cout << "ok." << endl;

        cout << "* Starting kaptivate capture... ";
        kap->startCapture(false);
        cout << "ok." << endl << endl;

        cout << "----------------------------------------------------------------------------" << endl;
        cout << "- Kaptivate started. Press ESC to stop.                                    -" << endl;
        cout << "----------------------------------------------------------------------------" << endl;

        WaitForSingleObject(stopEvent, INFINITE);
        CloseHandle(stopEvent);

        cout << "----------------------------------------------------------------------------" << endl;

        cout << endl;
        cout << "* Stopping kaptivate capture... ";
        kap->stopCapture();
        cout << "ok." << endl;

        cout << "* Destroying kaptivate instance... ";
        KaptivateAPI::destroyInstance();
        cout << "ok." << endl << endl;
    }
    catch(KaptivateException &ex)
    {
        cout << "Exception: " << ex.what() << endl;
    }

	return 0;
}
