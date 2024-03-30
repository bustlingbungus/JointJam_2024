#ifndef UNICODE
#define UNICODE
#endif

// macros
#define PLAYER 1
#define PLAYER_BULLET 2
#define MIN(a,b) (a<b)? a : b

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

// structs & classes
// stores x and y dimensions as floats
struct Vector2{
    float x = 0.0f;
    float y = 0.0f;

    // functions
    float length() { // ||v|| = sqrt(x^2 + y^2)
        return sqrt((x*x) + (y*y));
    }

    void normalise() { // makes itself into a unit vector, u = v / ||v||
        float len = length();
        x /= len; y /= len;
    }
};

// information about a game object
struct GameObject{
    int health;
    Vector2 pos;
    Vector2 velocity = {0.0f, 0.0f};
    float moveSpeed;
    int entityType;

    // rendering
    Gdiplus::Image * img;
    int size[2]; // image dimensions

    // constructors
    GameObject(Gdiplus::Image * image, int hp, float x, float y, float speed, int type) {
        img = image;
        size[0] = 30; size[1] = 30;
        health = hp;
        pos.x = x; pos.y = y;
        moveSpeed = speed;
        entityType = type;
    }
};

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
void copyOffscreenToWindow(HDC hdc);
void createBufferFrame(HWND hwnd);

float DeltaTime(); // time elapsed between frames

// drawing
void drawGameObject(GameObject * obj, Gdiplus::Graphics * graphics);
void drawBackgroundSection(Gdiplus::Graphics& graphics, Gdiplus::Image* image);

// game objects
void updateVelocities();
void updatePositions();
void updateGameObjects();
void deallocateGameObjects();

// conversions/logic
Vector2 getWorldSpaceCoords(float x, float y); // converts from window coordinates to corridinates in game
Gdiplus::Point getScreenCoords(float x, float y);