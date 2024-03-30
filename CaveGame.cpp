#include "CaveGame.hpp"

// globals
int wndWidth, wndHeight; // dimensions of window
float deltaTime = 0.0f, // time elapsed between frames
fixedDeltaTime = 16.0f; // 60fps
float timer = 0.0f; // time spent in the cave
bool gameIsPaused = 0;
// flashlight
float flashRange = 300.0f, flashWidth = 0.5f; // range of the flashlight
float ambientLightPercent = 0.3f, flashlightBrightness = 0.75f; // 0 to 1, how bright the scene/flashlight are

// gdiplus
Gdiplus::Image * background;
Gdiplus::Image * bulletImg;
Gdiplus::Image * Wall0Img;
int bkgWidth, bkgHeight;

// game objects
std::vector<GameObject*> gameObjects;
GameObject * player;
uint8 movementKeys = 0; // 0000wasd
Vector2 playerToMouse = {1,0};

// for functions
clock_t begin_time = clock(); // for tracking deltaTime

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

    HBRUSH bkg = CreateSolidBrush(RGB(255,255,255)); // window background color, white

    // load background
    background = Gdiplus::Image::FromFile(L"Background.png");
    bkgWidth = background->GetWidth(); bkgHeight = background->GetHeight();
    // bullet texture
    bulletImg = Gdiplus::Image::FromFile(L"Bullet.png");
    Wall0Img = Gdiplus::Image::FromFile(L"Wall0.png");

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
        (GetSystemMetrics(SM_CXFULLSCREEN)-900)/2, (GetSystemMetrics(SM_CYFULLSCREEN)-600)/2,
        900, 600,
        NULL,                   // parent window
        NULL,                   // menu
        hInstance,              // instance handle
        NULL                    // additional application data
    );
    if (hwnd == NULL) return 1; // validate window creation

    ShowWindow(hwnd, nCmdShow); // open the game window

    // place a wall in the map
    GameObject * wall = new GameObject(Wall0Img, 100, 100.0f, 100.0f, 0.0f, WALL, 600, 300);
    gameObjects.push_back(wall);

    // main message loop
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        // find time elapsed between frames
        deltaTime = DeltaTime();

        updateGameObjects();


        // increment timer
        if (!gameIsPaused) timer += 0.01f;

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
                case VK_ESCAPE:
                    gameIsPaused = !gameIsPaused;
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

        case WM_LBUTTONDOWN: {
            // get mouse coordinates on screen
            int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

            // shoot a bullet
            if (!gameIsPaused) shootBullet(x, y);
            break;
        }

        case WM_MOUSEMOVE: {// player moved mouse
            // get the mouse coordinates on screen
            int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

            if (gameIsPaused) break;
            // get coordinates in worldspace
            Vector2 mousePos = getWorldSpaceCoords((float)x, (float)y);
            // vector from player to mousePos
            playerToMouse = {mousePos.x-(player->pos.x+player->size[0]/2), mousePos.y-(player->pos.y+player->size[1]/2)};
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

    // draw background
    drawBackgroundSection(graphics, background);

    // draw game objects
    for (int i = 0; i < gameObjects.size(); i++) {
        drawGameObject(gameObjects[i], &graphics);
    }

    // flashlight
    illuminateFlashLight(graphics);

    if (gameIsPaused) {
        Gdiplus::Rect rect(0, 0, wndWidth, wndHeight);
        Gdiplus::SolidBrush pauseBrush(Gdiplus::Color(150, 0,0,0));
        graphics.FillRectangle(&pauseBrush, rect);
    }

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

            case PLAYER_BULLET: break; // constant velocity, no need to update

            case WALL:
                gameObjects[i]->velocity.x = 0.0f; gameObjects[i]->velocity.y = 0.0f;
                break;

            default: std::cout << "unknown entity: " << i << '\n'; break;
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
    if (gameIsPaused) return;
    updateVelocities();
    updatePositions();
    handleCollisions();
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

void illuminateFlashLight(Gdiplus::Graphics& graphics)
{
    // define vertices for triangle
    Gdiplus::Point playerPos = getScreenCoords(player->pos.x+(player->size[0]/2), player->pos.y+(player->size[1]/2)),
    bisector(INT(playerPos.X+(flashRange*playerToMouse.x)), INT(playerPos.Y+(flashRange*playerToMouse.y))),
    p1(INT(bisector.X-(playerToMouse.y*flashRange*flashWidth)), INT(bisector.Y+(playerToMouse.x*flashRange*flashWidth))),
    p2(INT(bisector.X+(playerToMouse.y*flashRange*flashWidth)), INT(bisector.Y-(playerToMouse.x*flashRange*flashWidth)));

    // vertices for the flashlight triangle
    Gdiplus::Point flashlightVertices[3] = {playerPos, p1, p2};

    // create a GraphicsPath to represent the triangle
    Gdiplus::GraphicsPath path;
    path.AddPolygon(flashlightVertices, 3);
    // create a region from the GraphicsPath
    Gdiplus::Region region(&path);
    // create a rectangle representing the entire window
    Gdiplus::Rect rect(0, 0, wndWidth, wndHeight);
    // create a region from the rectangle
    Gdiplus::Region windowRegion(rect);

    Gdiplus::SolidBrush flashlightBrush(Gdiplus::Color(int(255.0f*(1.0f-flashlightBrightness)), 0,0,0));
    graphics.FillRectangle(&flashlightBrush, rect);
    
    // exclude the triangular region from the window region
    windowRegion.Exclude(&region);

    // cover screen in black
    Gdiplus::SolidBrush blackBrush(Gdiplus::Color(int(255.0f*(1.0f-ambientLightPercent)), 0, 0, 0));
    graphics.FillRegion(&blackBrush, &windowRegion);
}

void shootBullet(int x, int y)
{
    // get position in world space
    Vector2 dest = getWorldSpaceCoords((float)x, (float)y);

    // vector from player to bullet destination
    dest.x -= player->pos.x+player->size[0]/2;
    dest.y -= player->pos.y+player->size[1]/2;

    // normalised
    dest.normalise();

    // instantiate a bullet on the player moving in the direction of dest
    GameObject * bullet = new GameObject(bulletImg, 0,
        player->pos.x+player->size[0]/2-10, player->pos.y+player->size[1]/2-10,
        400.0f, PLAYER_BULLET, 400.0f*dest.x, 400.0f*dest.y);
    gameObjects.push_back(bullet);
}

void handleCollisions()
{
    for (int i = 0; i < gameObjects.size(); i++)
    {
        // other objects collide with walls, not the other way around
        if (gameObjects[i]->entityType == WALL) continue;
        // hitbox for gameObjects[i]
        RECT obj0 = {(LONG)gameObjects[i]->pos.x,                         (LONG)gameObjects[i]->pos.y, 
                     LONG(gameObjects[i]->pos.x+gameObjects[i]->size[0]), LONG(gameObjects[i]->pos.y+gameObjects[i]->size[1])};

        // when collision is detected: if object is a bullet -> delete obj, if target is not wall or player -> delete target

        for (int j = 0; j < gameObjects.size(); j++)
        {
            //std::cout << i <<' '<< j <<'\n';
            if (i == j) continue; // object doesn't collide with itself
            // hitbox for gameObjects[j]
            RECT obj1 = {(LONG)gameObjects[j]->pos.x,                         (LONG)gameObjects[j]->pos.y, 
                         LONG(gameObjects[j]->pos.x+gameObjects[j]->size[0]), LONG(gameObjects[j]->pos.y+gameObjects[j]->size[1])};

            // left side of obj0 colliding
            if (obj0.left<obj1.right && obj0.right>obj1.right) {
                //   side collision,                                  obj0 bigger than wall side
                if ((obj0.top>obj1.top && obj0.bottom<obj1.bottom) || (obj0.top<obj1.top && obj0.bottom>obj1.bottom)) {
                    // if obj0 0 is a bullet
                    if (gameObjects[i]->entityType == PLAYER_BULLET) {
                        int res = bulletHit(gameObjects[i], gameObjects[j], &i, &j);
                        if (res == 0) continue;
                        else break;
                    } else gameObjects[i]->pos.x = obj1.right;
                } else {
                    // overlaps
                    int dTop    = (obj1.bottom-obj0.top)*(obj0.bottom>obj1.bottom),
                        dBottom = (obj0.bottom-obj1.top)*(obj0.top<obj1.top),
                        dLeft   = obj1.right-obj0.left;
                    if (dTop>0) { // top of obj0 collides with bottom left corner
                        // if x overlap is greater, the TOP is colliding, move along x axis
                        if (gameObjects[i]->entityType == PLAYER_BULLET) {
                            int res = bulletHit(gameObjects[i], gameObjects[j], &i, &j);
                            if (res == 0) continue;
                            else break;
                        } else if (dLeft>dTop) gameObjects[i]->pos.y = obj1.bottom;
                        else gameObjects[i]->pos.x = obj1.right;
                    } else if (dBottom>0) {
                        if (gameObjects[i]->entityType == PLAYER_BULLET) {
                            int res = bulletHit(gameObjects[i], gameObjects[j], &i, &j);
                            if (res == 0) continue;
                            else break;
                        } else if (dLeft>dBottom) gameObjects[i]->pos.y = obj1.top-gameObjects[i]->size[1];
                        else gameObjects[i]->pos.x = obj1.right;
                    }
                }
            } else if (obj0.right>obj1.left && obj0.left<obj1.left) { // right of obj0 colliding
                if ((obj0.top>obj1.top && obj0.bottom<obj1.bottom) || (obj0.top<obj1.top && obj0.bottom>obj1.bottom)) {
                    if (gameObjects[i]->entityType == PLAYER_BULLET) {
                        int res = bulletHit(gameObjects[i], gameObjects[j], &i, &j);
                        if (res == 0) continue;
                        else break;
                    } else gameObjects[i]->pos.x = obj1.left-gameObjects[i]->size[0];
                } else {
                    // overlaps
                    int dTop    = (obj1.bottom-obj0.top)*(obj0.bottom>obj1.bottom),
                        dBottom = (obj0.bottom-obj1.top)*(obj0.top<obj1.top),
                        dRight  = obj0.right-obj1.left;
                    if (dTop>0) { // top of obj0 collides with bottom left corner
                        if (gameObjects[i]->entityType == PLAYER_BULLET) {
                            int res = bulletHit(gameObjects[i], gameObjects[j], &i, &j);
                            if (res == 0) continue;
                            else break;
                        } else if (dRight>dTop) gameObjects[i]->pos.x = obj1.bottom;
                    } else if (dBottom>0) {
                        if (gameObjects[i]->entityType == PLAYER_BULLET) {
                            int res = bulletHit(gameObjects[i], gameObjects[j], &i, &j);
                            if (res == 0) continue;
                            else break;
                        } else if (dRight>dBottom) gameObjects[i]->pos.y = obj1.top-gameObjects[i]->size[1];
                        else gameObjects[i]->pos.x = obj1.left-gameObjects[i]->size[0];
                    }
                }
            } else if (obj0.bottom>obj1.top && obj0.top<obj1.top) { // bottom of obj0 colliding
                if ((obj0.left>obj1.left&&obj0.right<obj1.right) || (obj0.left<obj1.left&&obj0.right>obj1.right)) {
                    if (gameObjects[i]->entityType == PLAYER_BULLET) {
                        int res = bulletHit(gameObjects[i], gameObjects[j], &i, &j);
                        if (res == 0) continue;
                        else break;
                    } else gameObjects[i]->pos.y = obj1.top-gameObjects[i]->size[1];
                } // corner collisions handled in sides
            } else if (obj0.top<obj1.bottom&&obj0.bottom>obj1.bottom) { // bottom of obj0 colliding
                if ((obj0.left>obj1.left&&obj0.right<obj1.right) || (obj0.left<obj1.left&&obj0.right>obj1.right)) {
                    if (gameObjects[i]->entityType == PLAYER_BULLET) {
                        int res = bulletHit(gameObjects[i], gameObjects[j], &i, &j);
                        if (res == 0) continue;
                        else break;
                    } else gameObjects[i]->pos.y = obj1.bottom;
                }
            }
        }
    }
}

int bulletHit(GameObject* obj0, GameObject* obj1, int* i, int* j)
{
    // return codes: 0: hit player/other bullet, continue
    //               1: hit wall, delete self, break
    //               2: hit enemy, delete self, reduce hp from target, break
    // obj0->entityType == PLAYER_BULLET
    if (obj1->entityType==PLAYER_BULLET || obj1->entityType==PLAYER) return 0;
    if (obj1->entityType==WALL) {
        // delete self
        delete obj0;
        gameObjects.erase(gameObjects.begin() + (*i)--);
        return 1;
    } else {
        // reduce target hp
        obj1->health -= obj0->health;
        // delete self
        delete obj0;
        gameObjects.erase(gameObjects.begin() + (*i)--);
        // if target has no more hp, delete
        if (obj1->health <= 0) {
            delete obj1;
            gameObjects.erase(gameObjects.begin() + (*j)--);
        }
        return 2;
    }
}
