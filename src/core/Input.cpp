#include "Input.h"
#include <cstring>
#include <iostream>

bool Input::sKeysDown[MAX_KEYS];
bool Input::sKeysPressed[MAX_KEYS];
bool Input::sKeysPrevious[MAX_KEYS];

bool Input::sMouseButtons[5];
int Input::sMouseX = 0;
int Input::sMouseY = 0;
int Input::sMouseDX = 0;
int Input::sMouseDY = 0;
float Input::sMouseWheel = 0.0f;
bool Input::sRelativeMouseMode = false;

void Input::Init() {
    std::memset(sKeysDown, 0, sizeof(sKeysDown));
    std::memset(sKeysPressed, 0, sizeof(sKeysPressed));
    std::memset(sKeysPrevious, 0, sizeof(sKeysPrevious));
    std::memset(sMouseButtons, 0, sizeof(sMouseButtons));
}

void Input::Update() {
    // Actualizar estado de teclas para detectar "pressed"
    for (int i = 0; i < MAX_KEYS; ++i) {
        sKeysPressed[i] = sKeysDown[i] && !sKeysPrevious[i];
        sKeysPrevious[i] = sKeysDown[i];
    }

    // Reset deltas
    sMouseDX = 0;
    sMouseDY = 0;
    sMouseWheel = 0.0f;
}

void Input::ProcessEvent(const SDL_Event& e) {
    switch (e.type) {
    case SDL_EVENT_KEY_DOWN: {
        int code = (int)e.key.key;
        if (code >= 0 && code < MAX_KEYS) {
            sKeysDown[code] = true;
        }
        break;
    }

    case SDL_EVENT_KEY_UP: {
        int code = (int)e.key.key;
        if (code >= 0 && code < MAX_KEYS) {
            sKeysDown[code] = false;
        }
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        if (e.button.button < 5) {
            sMouseButtons[e.button.button] = true;

            // Clic derecho activa modo cámara
            if (e.button.button == SDL_BUTTON_RIGHT) {
                SDL_SetWindowRelativeMouseMode(SDL_GetMouseFocus(), true);
                sRelativeMouseMode = true;
                std::cout << "Camera control: ENABLED (Right mouse button)" << std::endl;
            }
        }
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_UP: {
        if (e.button.button < 5) {
            sMouseButtons[e.button.button] = false;

            // Soltar clic derecho desactiva modo cámara
            if (e.button.button == SDL_BUTTON_RIGHT) {
                SDL_SetWindowRelativeMouseMode(SDL_GetMouseFocus(), false);
                sRelativeMouseMode = false;
                std::cout << "Camera control: DISABLED" << std::endl;
            }
        }
        break;
    }

    case SDL_EVENT_MOUSE_MOTION: {
        sMouseX = (int)e.motion.x;
        sMouseY = (int)e.motion.y;
        sMouseDX += (int)e.motion.xrel;
        sMouseDY += (int)e.motion.yrel;
        break;
    }

    case SDL_EVENT_MOUSE_WHEEL: {
        sMouseWheel = e.wheel.y;
        break;
    }
    }
}

bool Input::IsKeyDown(SDL_Keycode key) {
    int code = (int)key;
    if (code >= 0 && code < MAX_KEYS) {
        return sKeysDown[code];
    }
    return false;
}

bool Input::IsKeyPressed(SDL_Keycode key) {
    int code = (int)key;
    if (code >= 0 && code < MAX_KEYS) {
        return sKeysPressed[code];
    }
    return false;
}

bool Input::IsMouseButtonDown(int button) {
    if (button >= 0 && button < 5) {
        return sMouseButtons[button];
    }
    return false;
}

void Input::GetMousePosition(int& x, int& y) {
    x = sMouseX;
    y = sMouseY;
}

void Input::GetMouseDelta(int& dx, int& dy) {
    dx = sMouseDX;
    dy = sMouseDY;
}

float Input::GetMouseWheelDelta() {
    return sMouseWheel;
}

bool Input::IsCameraControlActive() {
    return sRelativeMouseMode;
}