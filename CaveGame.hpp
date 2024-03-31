#ifndef UNICODE
#define UNICODE
#endif

// macros
#define PLAYER 1
#define PLAYER_BULLET 2
#define WALL 3
#define BATTERY 4
#define GEM 5
#define AMMO 6
#define ENEMY 7

#define LEFT 1
#define RIGHT 2
#define UP 3
#define DOWN 4

#define RANGE 1
#define WIDTH 2
#define BULLET_COUNT 3
#define CHARGE 4

#define PAUSE 0
#define VICTORY 1
#define LOSS 2

#define MIN(a,b) (a<b)? a : b
#define MAX(a,b) (a>b)? a : b
// typedefs
typedef unsigned char uint8; // 8 bit unsigned integer

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
#include <stack>
#include <vector>
#include <string>
#include <unordered_map>
#include <ctime>
#include <cmath>

#pragma comment (lib, "Gdiplus.lib")

// structs & classes
// stores x and y dimensions as floats
struct Vector2{
    float x = 0.0f;
    float y = 1.0f;

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
    // bullets wont have health, so hp can be used to identify how much damage a bullet does
    int health;
    Vector2 pos;
    Vector2 velocity = {0.0f, 0.0f};
    float moveSpeed;
    int entityType;
    bool idle;

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

    GameObject(Gdiplus::Image * image, int hp, float x, float y, float speed, int type, float velX, float velY) {
        img = image;
        size[0] = 20; size[1] = 20;
        health = hp;
        pos.x = x; pos.y = y;
        moveSpeed = speed;
        entityType = type;
        velocity.x = velX; velocity.y = velY;
    }

    GameObject(Gdiplus::Image * image, int hp, float x, float y, float speed, int type, int sizeX, int sizeY) {
        img = image;
        size[0] = sizeX; size[1] = sizeY;
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
void loadImages();

float DeltaTime(); // time elapsed between frames

// drawing
void drawGameObject(GameObject * obj, Gdiplus::Graphics * graphics);
void drawBackgroundSection(Gdiplus::Graphics& graphics, Gdiplus::Image* image);
void illuminateFlashLight(Gdiplus::Graphics& graphics);
// text
void placeText(int x, int y, std::wstring text, Gdiplus::Color color, int size, Gdiplus::Graphics& graphics);
void drawPauseMenuUI(Gdiplus::Graphics& graphics, int state);

// game objects
void shootBullet(int x, int y);
void generateEnemies(int n);
void updateVelocities();
void updatePositions();
void updateGameObjects();

void placeWalls();

void drainLight();

void handleCollisions();
int bulletHit(GameObject* obj0, GameObject* obj1, int* i, int* j);
void checkidle();
void pickUpItem(GameObject* obj0, GameObject* obj1, int* j);

// conversions/logic
Vector2 getWorldSpaceCoords(float x, float y); // converts from window coordinates to corridinates in game
Gdiplus::Point getScreenCoords(float x, float y);
int loadGlobals();
int saveGlobals();
void interactWithPauseMenu(int x, int y, HWND hwnd);
void improveStat(int stat);

// generation
std::unordered_map<Vector2*, float> generateWalls();
void placeItems();
void generateRoom(Vector2 playerPos);