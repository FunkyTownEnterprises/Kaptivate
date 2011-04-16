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
#include "resource.hpp"
#include "sample_app.hpp"

#include <vector>
#include <iostream>
using namespace std;

#include "kaptivate.hpp"
#include "kaptivate_exceptions.hpp"
using namespace Kaptivate;

#ifdef UNICODE
#define stringcopy wcscpy
#else
#define stringcopy strcpy
#endif

#define ID_TRAY_APP_ICON                 5000
#define ID_TRAY_EXIT_CONTEXT_MENU_ITEM   3000
#define ID_TRAY_PAUSE_CONTEXT_MENU_ITEM  3001
#define ID_TRAY_RESUME_CONTEXT_MENU_ITEM 3002
#define WM_TRAYICON (WM_USER + 1)

UINT WM_TASKBARCREATED = 0 ;
HWND g_hwnd;
HMENU g_menu;
NOTIFYICONDATA g_notifyIconData ;

class SpaceEater : public KeyboardHandler
{
public:
    virtual void HandleKeyEvent(KeyboardEvent& evt)
    {
        // Decide whether or not to consume the keystroke
        if(evt.getVkey() == VK_SPACE)
            evt.setDecision(CONSUME);
    }
};

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        g_menu = CreatePopupMenu();
        AppendMenu(g_menu, MF_STRING | MF_GRAYED,
            ID_TRAY_RESUME_CONTEXT_MENU_ITEM, TEXT("&Resume"));
        AppendMenu(g_menu, MF_STRING, ID_TRAY_PAUSE_CONTEXT_MENU_ITEM, TEXT("&Pause"));
        AppendMenu(g_menu, MF_SEPARATOR, NULL, NULL);
        AppendMenu(g_menu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM,  TEXT("E&xit"));

        MessageBoxEx(hwnd, TEXT("Kaptivate is running! No more spaces for you.\nUse the icon in the system tray to exit."),
            TEXT("Kaptivate"), MB_OK, 0);
        break;

    case WM_TRAYICON:
        {
            if (lParam == WM_LBUTTONUP)
            {
                //printf( "You have restored me!\n" ) ;
                //Restore();
            }
            else if (lParam == WM_RBUTTONUP)
            {
                POINT curPoint ;
                GetCursorPos(&curPoint) ;
                SetForegroundWindow(hwnd); 
                UINT clicked = TrackPopupMenu(g_menu, TPM_RETURNCMD | TPM_NONOTIFY,
                    curPoint.x, curPoint.y, 0, hwnd, NULL);

                if(clicked == ID_TRAY_EXIT_CONTEXT_MENU_ITEM)
                {
                    // quit the application.
                    PostQuitMessage(0);
                }
                else if(clicked == ID_TRAY_PAUSE_CONTEXT_MENU_ITEM)
                {
                    try
                    {
                        KaptivateAPI* kaptivate = KaptivateAPI::getInstance();
                        kaptivate->suspendCapture();
                        if(kaptivate->isSuspended())
                        {
                            EnableMenuItem(g_menu, ID_TRAY_PAUSE_CONTEXT_MENU_ITEM, MF_GRAYED);
                            EnableMenuItem(g_menu, ID_TRAY_RESUME_CONTEXT_MENU_ITEM, MF_ENABLED);
                        }
                    }
                    catch(KaptivateException)
                    {
                        MessageBoxEx(hwnd, TEXT("Unable to pause."), TEXT("Kaptivate"), MB_OK, 0);
                    }
                }
                else if(clicked == ID_TRAY_RESUME_CONTEXT_MENU_ITEM)
                {
                    try
                    {
                        KaptivateAPI* kaptivate = KaptivateAPI::getInstance();
                        kaptivate->resumeCapture();
                        if(!kaptivate->isSuspended())
                        {
                            EnableMenuItem(g_menu, ID_TRAY_PAUSE_CONTEXT_MENU_ITEM, MF_ENABLED);
                            EnableMenuItem(g_menu, ID_TRAY_RESUME_CONTEXT_MENU_ITEM, MF_GRAYED);
                        }
                    }
                    catch(KaptivateException)
                    {
                        MessageBoxEx(hwnd, TEXT("Unable to resume."), TEXT("Kaptivate"), MB_OK, 0);
                    }
                }
            }
        }
        break;

    case WM_CLOSE:
        //Minimize() ;
        return 0;

    case WM_DESTROY:
        printf( "DESTROY!!\n" ) ;
        PostQuitMessage (0);
        break;
    }

    return DefWindowProc( hwnd, message, wParam, lParam ) ;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR args, int iCmdShow)
{
    SpaceEater spaceHandler;
    KaptivateAPI* kaptivate = NULL;

    try
    {
        // Set up our win32 stuff to process the trayicon
        {
            TCHAR className[] = TEXT( "tray icon class" );
            WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated") ;

            WNDCLASSEX wnd = { 0 };

            wnd.hInstance = hInstance;
            wnd.lpszClassName = className;
            wnd.lpfnWndProc = WndProc;
            wnd.style = CS_HREDRAW | CS_VREDRAW ;
            wnd.cbSize = sizeof (WNDCLASSEX);
            wnd.hIcon = NULL;
            wnd.hIconSm = NULL;
            wnd.hCursor = NULL;
            wnd.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);

            if(!RegisterClassEx(&wnd))
                FatalAppExit(0, TEXT("Couldn't register window class!"));

            g_hwnd = CreateWindowEx(NULL, className, NULL, NULL, CW_USEDEFAULT, CW_USEDEFAULT,
                0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

            memset(&g_notifyIconData, 0, sizeof(NOTIFYICONDATA));
            g_notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
            g_notifyIconData.hWnd = g_hwnd;
            g_notifyIconData.uID = ID_TRAY_APP_ICON;
            g_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            g_notifyIconData.uCallbackMessage = WM_TRAYICON;
            g_notifyIconData.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
            stringcopy(g_notifyIconData.szTip, TEXT("Kaptivate"));
            Shell_NotifyIcon(NIM_ADD, &g_notifyIconData);
        }

        // Set up Kaptivate
        kaptivate = KaptivateAPI::getInstance();
        kaptivate->enumerateKeyboards();
        kaptivate->registerKeyboardHandler(".*", &spaceHandler);
        kaptivate->startCapture(false, true);

        MSG msg;
        while(GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        kaptivate->stopCapture();
        KaptivateAPI::destroyInstance();

        Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData);

        return msg.wParam;
    }
    catch(KaptivateException &kex)
    {
        char msg[4096];
        wchar_t wmsg[4096];
        sprintf(msg, "Kaptivate exception: %s", kex.what());
        mbstowcs(wmsg, msg, 4096);

        FatalAppExit(0, wmsg);
    }

    return 1;
}
