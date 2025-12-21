#pragma once
#include "Window.h"
#include "Rendering/Renderer.h"
#include "Rendering/Camera.h"
#include <string>

class Application {
public:
    Application();
    ~Application();
    void Run();

private:
    Window* window = nullptr;
    Camera* camera = nullptr;

    float mClearColor[4] = { 0.1f, 0.1f, 0.15f, 1.0f }; // color inicial


    bool modelLoadedLastFrame = false; // NUEVO
};