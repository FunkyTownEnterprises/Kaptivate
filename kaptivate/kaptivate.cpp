/*
 * Kaptivate.cpp
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
#include "kaptivate.h"
#include "kaptivate_exceptions.h"

#include <iostream>
using namespace std;
using namespace Kaptivate;

// Singleton static members
bool KaptivateAPI::instanceFlag       = false;
KaptivateAPI* KaptivateAPI::singleton = NULL;

KaptivateAPI::KaptivateAPI()
{
}

KaptivateAPI::~KaptivateAPI()
{
}

// Get an instance of this thing
KaptivateAPI* KaptivateAPI::getInstance()
{
    if(!instanceFlag)
    {
        try
        {
            if(NULL == (singleton = new KaptivateAPI()))
                throw bad_alloc();
        }
        catch(bad_alloc&)
        {
            throw KaptivateException("Unable to allocate memory for the Kaptivate singleton");
        }

        instanceFlag = true;
    }

    return singleton;
}

// Destroy the singleton
void KaptivateAPI::destroyInstance()
{
    if(!instanceFlag || singleton == NULL)
        return;

    delete singleton;
    singleton = NULL;

    instanceFlag = false;
}
