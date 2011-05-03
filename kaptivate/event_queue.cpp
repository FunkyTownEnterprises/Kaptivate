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

#include "stdafx.hpp"
#include "event_queue.hpp"
#include "scoped_mutex.hpp"

using namespace Kaptivate;

// http://www.tidytutorials.com/2009/10/windows-c-producer-consumer-threaded.html
// http://msdn.microsoft.com/en-us/library/ms687025(v=VS.85).aspx
// http://dev-faqs.blogspot.com/2011/03/revisiting-producer-and-consumer.html

EventQueue::EventQueue()
{
    this->kbdEventSignal = CreateEvent(NULL, TRUE, FALSE, NULL);
    this->kbdStopSignal = CreateEvent(NULL, TRUE, FALSE, NULL);
    this->kbdHandles[0] = this->kbdEventSignal;
    this->kbdHandles[1] = this->kbdStopSignal;

    this->mouseEventSignal = CreateEvent(NULL, TRUE, FALSE, NULL);
    this->mouseStopSignal = CreateEvent(NULL, TRUE, FALSE, NULL);
    this->mouseHandles[0] = this->mouseEventSignal;
    this->mouseHandles[1] = this->mouseStopSignal;

    this->stopped = true;

    InitializeCriticalSection(&mouseQueueLock);
    InitializeCriticalSection(&kbdQueueLock);
}

EventQueue::~EventQueue()
{
    stop();

    CloseHandle(this->kbdEventSignal);
    CloseHandle(this->kbdStopSignal);
    CloseHandle(this->mouseEventSignal);
    CloseHandle(this->mouseStopSignal);

    this->kbdHandles[0] = 0;
    this->kbdHandles[1] = 0;
    this->mouseHandles[0] = 0;
    this->mouseHandles[1] = 0;

    DeleteCriticalSection(&mouseQueueLock);
    DeleteCriticalSection(&kbdQueueLock);
}

void EventQueue::start()
{
    ScopedCriticalSection crit1(&mouseQueueLock);
    ScopedCriticalSection crit2(&kbdQueueLock);
    stopped = false;
}

void EventQueue::stop()
{
    ScopedCriticalSection crit1(&mouseQueueLock);
    ScopedCriticalSection crit2(&kbdQueueLock);

    if(stopped)
        return;

    stopped = true;

    SetEvent(mouseStopSignal);
    SetEvent(kbdStopSignal);
}

bool EventQueue::running()
{
    ScopedCriticalSection crit1(&mouseQueueLock);
    ScopedCriticalSection crit2(&kbdQueueLock);
    return !stopped;
}

void EventQueue::EnqueueKeyboardEvent(KeyboardEvent* kbdEvent)
{
    ScopedCriticalSection crit(&kbdQueueLock);
    if(stopped)
        return;
    kbEventQueue.push(kbdEvent);
    SetEvent(kbdEventSignal);
}

KeyboardEvent* EventQueue::DequeueKeyboardEvent()
{
    ScopedCriticalSection crit(&kbdQueueLock);
    if(stopped)
        return NULL;

    while(kbEventQueue.empty() && !stopped)
    {
        {
            ScopedNonCriticalSection uncrit(&kbdQueueLock);
            if((DWORD)kbdEventSignal != WaitForMultipleObjects(2, kbdHandles, FALSE, INFINITE))
                return NULL;
        }
    }

    KeyboardEvent* evt = kbEventQueue.front();
    kbEventQueue.pop();
    return evt;
}

void EventQueue::EnqueueMouseButtonEvent(MouseButtonEvent* mbEvent)
{
    ScopedCriticalSection crit(&mouseQueueLock);
    if(stopped)
        return;
    mbEventQueue.push(mbEvent);
    SetEvent(mouseEventSignal);
}

MouseButtonEvent* EventQueue::DequeueMouseButtonEvent()
{
    ScopedCriticalSection crit(&mouseQueueLock);

    if(stopped)
        return NULL;

    while(mbEventQueue.empty() && !stopped)
    {
        {
            ScopedNonCriticalSection uncrit(&mouseQueueLock);
            if((DWORD)mouseEventSignal != WaitForMultipleObjects(2, mouseHandles, FALSE, INFINITE))
                return NULL;
        }
    }

    MouseButtonEvent* evt = mbEventQueue.front();
    mbEventQueue.pop();

    return evt;
}

void EventQueue::EnqueueMouseWheelEvent(MouseWheelEvent* mwEvent)
{
}

MouseWheelEvent* EventQueue::DequeueMouseWheelEvent()
{
    return NULL;
}

void EventQueue::EnqueueMouseMoveEvent(MouseMoveEvent* mmEvent)
{
}

MouseMoveEvent* EventQueue::DequeueMouseMoveEvent()
{
    return NULL;
}
