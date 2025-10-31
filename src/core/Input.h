#pragma once
#include <SDL3/SDL.h>

class Input {
public:
    static void Init();
    static void Update();
    static void ProcessEvent(const SDL_Event& e);

    // Teclado
    static bool IsKeyDown(SDL_Keycode key);
    static bool IsKeyPressed(SDL_Keycode key);

    // Ratón
    static bool IsMouseButtonDown(int button);
    static void GetMousePosition(int& x, int& y);
    static void GetMouseDelta(int& dx, int& dy);
    static float GetMouseWheelDelta();

    // Estado de la cámara
    static bool IsCameraControlActive();

private:
    static const int MAX_KEYS = 512;
    static bool sKeysDown[MAX_KEYS];
    static bool sKeysPressed[MAX_KEYS];
    static bool sKeysPrevious[MAX_KEYS];

    static bool sMouseButtons[5];
    static int sMouseX, sMouseY;
    static int sMouseDX, sMouseDY;
    static float sMouseWheel;
    static bool sRelativeMouseMode;
}; 

