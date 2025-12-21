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

    bool modelLoadedLastFrame = false; // NUEVO
};