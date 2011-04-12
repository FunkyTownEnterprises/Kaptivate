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

// Extend the keyboard handler. Must implement HandleKeyEvent
class MyKeyboardHandler : public KeyboardHandler
{
private:
    HANDLE stopEvent;

public:
    MyKeyboardHandler(HANDLE stopEvent)
    {
        this->stopEvent = stopEvent;
    }

    virtual void HandleKeyEvent(KeyboardEvent& evt)
    {
        // Show some info about the event
        cout << "Got a keyboard event:" << endl;
        cout << " device: " << evt.getDeviceInfo()->name << endl;
        cout << " vkey: " << evt.getVkey() << endl;
        cout << " scan code: " << evt.getScanCode() << endl;
        cout << " key up?: " << evt.getKeyUp() << endl;

        // Decide whether or not to consume the keystroke
        if(evt.getVkey() == VK_SPACE)
            evt.setDecision(CONSUME);
        else
            evt.setDecision(PERMIT);

        // Print out the decision
        cout << " decision: ";
        switch(evt.getDecision())
        {
        case UNDECIDED:
            cout << "undecided" << endl;
            break;
        case PERMIT:
            cout << "permit" << endl;
            break;
        case CONSUME:
            cout << "consume" << endl;
            break;
        case PASS:
            cout << "pass" << endl;
            break;
        default:
            cout << "unknown" << endl;
        }

        cout << endl;
        if(evt.getVkey() == VK_ESCAPE)
            SetEvent(stopEvent);
    }
};

int _tmain(int argc, _TCHAR* argv[])
{
    KaptivateAPI* kap = NULL;

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

    cout << "* Registering keyboard handler... ";
    HANDLE stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    MyKeyboardHandler* handler = new MyKeyboardHandler(stopEvent);
    kap->registerKeyboardHandler(".*", handler);
    cout << "ok." << endl;

    cout << "* Starting kaptivate capture... ";
    kap->startCapture(false);
    cout << "ok." << endl << endl;

    cout << "----------------------------------------------------------------------------" << endl;
    cout << "- Kaptivate started. Press ESC to stop.                                    -" << endl;
    cout << "----------------------------------------------------------------------------" << endl;

    WaitForSingleObject(stopEvent, INFINITE);
    CloseHandle (stopEvent);

    cout << "----------------------------------------------------------------------------" << endl;

    cout << endl;
    cout << "* Stopping kaptivate capture... ";
    kap->stopCapture();
    cout << "ok." << endl;

    cout << "* Destroying kaptivate instance... ";
    KaptivateAPI::destroyInstance();
    cout << "ok." << endl;
    }
    catch(KaptivateException &ex)
    {
        cout << "Exception: " << ex.what() << endl;
    }

	return 0;
}
