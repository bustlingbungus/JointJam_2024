#include "CaveGame.hpp"

// globals
int wndWidth, wndHeight; // dimensions of window
float deltaTime = 0.0f, // time elapsed between frames
fixedDeltaTime = 16.0f; // 60fps
float flashRange = 300.0f; // range of the flashlight
float ambientLightPercent = 0.1f; // 0 to 1, how bright the scene is without flashlight

// gdiplus
Gdiplus::Image * background;
int bkgWidth, bkgHeight;

// game objects
std::vector<GameObject*> gameObjects;
GameObject * player;
int movementKeys = 0; // 0000wasd
Vector2 playerToMouse = {1,0};


// for functions
clock_t begin_time = clock(); // for tracting deltaTime

// windows
HBITMAP hOffscreenBitmap; // buffer frame not seen by user
HDC hOffscreenDC, g_hdc;  // DC for offscreen device context

// main window display function
int WINAPI wndMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // initialise GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    HBRUSH bkg = CreateSolidBrush(RGB(150,255,150)); // window background color, white

    // load background
    background = Gdiplus::Image::FromFile(L"Background.png");
    bkgWidth = background->GetWidth(); bkgHeight = background->GetHeight();

    // instantiate player object
    GameObject obj(Gdiplus::Image::FromFile(L"Player.png"),
        10, 10.0f, 10.0f, 200.0f, PLAYER);
    player = &obj;
    gameObjects.push_back(player);

    // register window class
    const wchar_t CLASS_NAME[] = L"Window Class";
    WNDCLASS wc = { };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = bkg;
    RegisterClass(&wc);

    // create the window
    HWND hwnd = CreateWindowEx(
        0,                      // optional window styles
        CLASS_NAME,             // window class
        L"Cave Game",           // window text
        WS_OVERLAPPEDWINDOW,    // window style
        // position and size, center of screen, 750x500 (2:3)
        (GetSystemMetrics(SM_CXFULLSCREEN)-750)/2, (GetSystemMetrics(SM_CYFULLSCREEN)-500)/2,
        750, 500,
        NULL,                   // parent window
        NULL,                   // menu
        hInstance,              // instance handle
        NULL                    // additional application data
    );
    if (hwnd == NULL) return 1; // validate window creation

    ShowWindow(hwnd, nCmdShow); // open the game window

    // main message loop
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        // find time elapsed between frames
        deltaTime = DeltaTime();

        updateGameObjects();

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // shut down GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);
    // cleanup
    DeleteObject(bkg);

    return 0;
}

// window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE: // window creation
            // get device context for the window
            InitialiseOffscreenDC(hwnd);
            break;

        case WM_PAINT: { // called continuously
            createBufferFrame(hwnd);
            // copy buffer frame to visible window
            copyOffscreenToWindow(g_hdc);
            break;
        }

        // W = 0x57, A = 0x41, S = 0x53, D = 0x44
        case WM_KEYDOWN:
            switch (wParam)
            {
                case 0x57: // w
                    movementKeys |= 8; break;
                case 0x41: // a
                    movementKeys |= 4; break;
                case 0x53: // s
                    movementKeys |= 2; break;
                case 0x44: // d
                    movementKeys |= 1; break;
            }
            break;

        case WM_KEYUP:
            switch (wParam)
            {
                case 0x57: // w
                    movementKeys ^= 8; break;
                case 0x41: // a
                    movementKeys ^= 4; break;
                case 0x53: // s
                    movementKeys ^= 2; break;
                case 0x44: // d
                    movementKeys ^= 1; break;
            }
            break;

        case WM_MOUSEMOVE: {// player moved mouse
            // get the mouse coordinates on screen
            int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

            // get coordinates in worldspace
            Vector2 mousePos = getWorldSpaceCoords((float)x, (float)y);
            // vector from player to mousePos
            playerToMouse = {mousePos.x-player->pos.x, mousePos.y-player->pos.y};
            // normalised
            playerToMouse.normalise();
            break;
        }

        case WM_DESTROY: // window closed
            // clean up offscreen resources
            DeleteObject(hOffscreenBitmap);
            DeleteDC(hOffscreenDC);
            ReleaseDC(hwnd, g_hdc);

            // deallocate other resources
            deallocateGameObjects();
            delete background;

            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void InitialiseOffscreenDC(HWND hwnd)
{
    // get window dimensions
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    wndWidth  = clientRect.right  - clientRect.left;
    wndHeight = clientRect.bottom - clientRect.top; 

    g_hdc = GetDC(hwnd); // assign global device context
    // create a compatible DC for the offscreen bitmap
    hOffscreenDC = CreateCompatibleDC(g_hdc);
    hOffscreenBitmap = CreateCompatibleBitmap(g_hdc, wndWidth, wndHeight);
    SelectObject(hOffscreenDC, hOffscreenBitmap);

    RECT rect = { 0, 0, wndWidth, wndHeight };
    HBRUSH bkg = CreateSolidBrush(RGB(150,255,150)); // just initially covers the window with a solid color
    FillRect(hOffscreenDC, &rect, bkg);
    DeleteObject(bkg);
}

void copyOffscreenToWindow(HDC hdc)
{
    BitBlt(hdc, 0, 0, wndWidth, wndHeight, hOffscreenDC, 0, 0, SRCCOPY);
}

void createBufferFrame(HWND hwnd)
{
    // update window dimensions
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    wndWidth  = clientRect.right  - clientRect.left;
    wndHeight = clientRect.bottom - clientRect.top; 

    Gdiplus::Graphics graphics(hOffscreenDC); // graphics object for drawing

    // define vertices for triangle
    Gdiplus::Point playerPos = getScreenCoords(player->pos.x+(player->size[0]/2), player->pos.y+(player->size[1]/2));
    Gdiplus::Point bisector(INT(playerPos.X+(flashRange*playerToMouse.x)), INT(playerPos.Y+(flashRange*playerToMouse.y)));
    Gdiplus::Point p1(INT(bisector.X-(playerToMouse.y*flashRange/2)), INT(bisector.Y+(playerToMouse.x*flashRange/2)));
    Gdiplus::Point p2(INT(bisector.X+(playerToMouse.y*flashRange/2)), INT(bisector.Y-(playerToMouse.x*flashRange/2)));

    // vertices for the flashlight triangle
    Gdiplus::Point flashlightVertices[3] = {playerPos, p1, p2};

    // draw background
    drawBackgroundSection(graphics, background);

    // draw game objects
    for (int i = 0; i < gameObjects.size(); i++) {
        drawGameObject(gameObjects[i], &graphics);
    }

    // create a GraphicsPath to represent the triangle
    Gdiplus::GraphicsPath path;
    path.AddPolygon(flashlightVertices, 3);
    // create a region from the GraphicsPath
    Gdiplus::Region region(&path);
    // create a rectangle representing the entire window
    Gdiplus::Rect rect(0, 0, wndWidth, wndHeight);
    // create a region from the rectangle
    Gdiplus::Region windowRegion(rect);
    
    // exclude the triangular region from the window region
    windowRegion.Exclude(&region);


    // cover screen in black
    Gdiplus::SolidBrush blackBrush(Gdiplus::Color(int(255.0f*(1.0f-ambientLightPercent)), 0, 0, 0));
    graphics.FillRegion(&blackBrush, &windowRegion);

    // deallocate resources
}

float DeltaTime()
{
    clock_t t = clock(); // current time
    float dt = float(t - begin_time) / CLOCKS_PER_SEC;
    begin_time = t; // update starting time
    return MIN(dt, fixedDeltaTime);
}

void drawGameObject(GameObject * obj, Gdiplus::Graphics * graphics)
{
    if (obj->img->GetLastStatus() == Gdiplus::Ok)
    {
        int pos0, pos1;
        int width = wndWidth/2, height = wndHeight/2; // half the width and height

        if      (player->pos.x < width || bkgWidth < wndWidth) pos0 = (int)obj->pos.x;
        else if (player->pos.x > bkgWidth-width)               pos0 = wndWidth+(int)obj->pos.x-bkgWidth;
        else    pos0 = (obj==player)?                          width : width-player->pos.x+obj->pos.x;
        if      (pos0 < -obj->size[0] || pos0>wndWidth) return; // off screen, don't render

        if      (player->pos.y < height || bkgHeight < wndHeight) pos1 = (int)obj->pos.y;
        else if (player->pos.y > bkgHeight-height)                pos1 = wndHeight+(int)obj->pos.y-bkgHeight;
        else    pos1 = (obj==player)?                             height : height-player->pos.y+obj->pos.y;
        if      (pos1 < -obj->size[1] || pos1 > wndWidth) return; // off screen, don't render

        graphics->DrawImage(obj->img, pos0, pos1, obj->size[0], obj->size[1]);
    } else std::cout << "error loading image\n";
}

void updateVelocities()
{
    for (int i = 0; i < gameObjects.size(); i++) {
        switch (gameObjects[i]->entityType)
        {
            case PLAYER:
                player->velocity.x = player->moveSpeed * (bool(movementKeys&1) - bool(movementKeys&4));
                player->velocity.y = player->moveSpeed * (bool(movementKeys&2) - bool(movementKeys&8));
                break;

            default: std::cout << "unknown entity\n"; break;
        }
    }
}

void updatePositions()
{
    for (int i = 0; i < gameObjects.size(); i++)
    {
        gameObjects[i]->pos.x += gameObjects[i]->velocity.x * deltaTime;
        gameObjects[i]->pos.y += gameObjects[i]->velocity.y * deltaTime;
    }
}

void updateGameObjects()
{
    updateVelocities();
    updatePositions();
}

void drawBackgroundSection(Gdiplus::Graphics& graphics, Gdiplus::Image* image)
{
    // offsets
    int src0 = 0, src1 = 0, bkgx = 0, bkgy = 0;

    if (bkgWidth<wndWidth) bkgx = (wndWidth-bkgWidth)/2;
    else if (player->pos.x<wndWidth/2) src0 = 0;
    else if (player->pos.x>bkgWidth-wndWidth/2) src0 = bkgWidth-wndWidth;
    else src0 = (int)player->pos.x - (wndWidth/2);

    if (bkgHeight<wndHeight) bkgy = (wndHeight-bkgHeight)/2;
    else if (player->pos.y<wndHeight/2) src1 = 0;
    else if (player->pos.y>bkgHeight-wndHeight/2) src1 = bkgHeight-wndHeight;
    else src1 = (int)player->pos.y - (wndHeight/2);

    // destination rectangle
    Gdiplus::Rect destRect(bkgx, bkgy, wndWidth, wndHeight);
    // source rectangle
    Gdiplus::Rect srcRect(src0, src1, wndWidth, wndHeight);

    // draw section
    graphics.DrawImage(image, destRect, srcRect.X, srcRect.Y, 
    srcRect.Width, srcRect.Height, Gdiplus::UnitPixel);
}

void deallocateGameObjects()
{
    for (int i = 0; i < gameObjects.size(); i++) {
        delete gameObjects[i]->img;
    }
}

// finds the in game coordinates for a position on the window
Vector2 getWorldSpaceCoords(float x, float y)
{
    int width = wndWidth/2, height = wndHeight/2; // half the width and height

    if (player->pos.x < width || bkgWidth < wndWidth);
    else if (player->pos.x > bkgWidth-width) x += bkgWidth-wndWidth;
    else x += player->pos.x-width;

    if (player->pos.y < height || bkgHeight < wndHeight);
    else if (player->pos.y > bkgHeight-height) y += bkgHeight-wndHeight;
    else y += player->pos.y-height;

    return Vector2 {x, y};
}

// finds the on screen coordinates of a world space coordinate
Gdiplus::Point getScreenCoords(float x, float y)
{
    int width = wndWidth/2, height = wndHeight/2; // half the width and height

    if (player->pos.x < width || bkgWidth < wndWidth);
    else if (player->pos.x > bkgWidth-width) x -= bkgWidth-wndWidth;
    else x -= player->pos.x-width;

    if (player->pos.y < height || bkgHeight < wndHeight);
    else if (player->pos.y > bkgHeight-height) y -= bkgHeight-wndHeight;
    else y -= player->pos.y-height;

    return Gdiplus::Point((INT)x, (INT)y);
}
