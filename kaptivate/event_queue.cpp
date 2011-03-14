/*
 * event_queue.cpp
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
#include "event_queue.h"

using namespace Kaptivate;

EventQueue::EventQueue()
{
}

EventQueue::~EventQueue()
{
}

void EventQueue::EnqueueKeyboardEvent(KeyboardEvent* kbdEvent)
{
    kbEventQueue.push(kbdEvent);
}

KeyboardEvent* EventQueue::DequeueKeyboardEvent()
{
    if(kbEventQueue.empty())
        return NULL;

    KeyboardEvent* evt = kbEventQueue.front();
    kbEventQueue.pop();

    return evt;
}

void EventQueue::EnqueueMouseButtonEvent(MouseButtonEvent* mbEvent)
{
    mbEventQueue.push(mbEvent);
}

MouseButtonEvent* EventQueue::DequeueMouseButtonEvent()
{
    if(mbEventQueue.empty())
        return NULL;

    MouseButtonEvent* evt = mbEventQueue.front();
    mbEventQueue.pop();

    return evt;
}

void EventQueue::EnqueueMouseWheelEvent(MouseWheelEvent* mwEvent)
{
    mwEventQueue.push(mwEvent);
}

MouseWheelEvent* EventQueue::DequeueMouseWheelEvent()
{
    if(mwEventQueue.empty())
        return NULL;

    MouseWheelEvent* evt = mwEventQueue.front();
    mwEventQueue.pop();

    return evt;
}

void EventQueue::EnqueueMouseMoveEvent(MouseMoveEvent* mmEvent)
{
    mmEventQueue.push(mmEvent);
}

MouseMoveEvent* EventQueue::DequeueMouseMoveEvent()
{
    if(mmEventQueue.empty())
        return NULL;

    MouseMoveEvent* evt = mmEventQueue.front();
    mmEventQueue.pop();

    return evt;
}
