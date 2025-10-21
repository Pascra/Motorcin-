#pragma once

#include "Window.h"
#include "Renderer.h"

class Application {
public:
    Application();
    ~Application();

    void Run();

private:
    Window* window;
    bool running = true;
};
