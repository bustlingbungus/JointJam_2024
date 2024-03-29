#ifndef UNICODE
#define UNICODE
#endif

// macros

/* 
REMEMBER TO LINK WITH -lgdi32 and -lgdiplus WHEN COMPILING !!!!!

    example command line:
    cd "folder path" ; if ($?) { g++ Runner.cpp -o Runner -lgdi32 -lgdiplus } ; if ($?) { .\Runner } 
    
    - remember to run Runner.cpp, not CaveGame.cpp
*/

// windows
#include <Windows.h>
#include <windowsx.h>
#include <WinUser.h>
// gdiplus
#include <gdiplus.h>

// std
#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>

// windows
int WINAPI wndMain( // main window display function
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    int nCmdShow);
// window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// offscreen
void InitialiseOffscreenDC(HWND hwnd);
void copyOffscreenToWindow(HWND hwnd, HDC hdc);
void createBufferFrame(HWND hwnd);