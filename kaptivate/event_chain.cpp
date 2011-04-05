/*
 * event_chain.cpp
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
#include "event_chain.hpp"

using namespace std;
using namespace Kaptivate;

KeyboardEventChain::KeyboardEventChain()
{
}

KeyboardEventChain::~KeyboardEventChain()
{
}

void KeyboardEventChain::clearHandlers()
{
    handlers.clear();
}

void KeyboardEventChain::addHandler(KeyboardHandler* handler)
{
    if(handler)
        handlers.insert(handlers.begin(), handler);
}

void KeyboardEventChain::removeHandler(KeyboardHandler* handler)
{
    vector<KeyboardHandler*>::iterator it;
    for(it = handlers.begin(); it != handlers.end(); it++)
    {
        if(*it == handler)
            handlers.erase(it);
    }
}

unsigned int KeyboardEventChain::chainSize()
{
    return (unsigned int)handlers.size();
}

////////////////////////////////////////////////////////////////////////////////

MouseEventChain::MouseEventChain()
{
}

MouseEventChain::~MouseEventChain()
{
}

void MouseEventChain::clearHandlers()
{
    handlers.clear();
}

void MouseEventChain::addHandler(MouseHandler* handler)
{
    if(handler)
        handlers.insert(handlers.begin(), handler);
}

void MouseEventChain::removeHandler(MouseHandler* handler)
{
    vector<MouseHandler*>::iterator it;
    for(it = handlers.begin(); it != handlers.end(); it++)
    {
        if(*it == handler)
            handlers.erase(it);
    }
}

unsigned int MouseEventChain::chainSize()
{
    return (unsigned int)handlers.size();
}
