// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

// Needed for global hooks
HMODULE __kaptivateDllModule = 0;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
        if(__kaptivateDllModule == 0)
            __kaptivateDllModule = hModule;
        break;
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
