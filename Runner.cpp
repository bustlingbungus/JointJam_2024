#include "CaveGame.cpp"

int main() {
    int ext = wndMain (
        GetModuleHandle(NULL),
        NULL,
        GetCommandLine(),
        SW_SHOWDEFAULT);

    return ext;
}