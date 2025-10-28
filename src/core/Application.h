#pragma once
#include "Window.h"
#include "Renderer.h"
#include <string>

class Application {
public:
    Application();
    ~Application();
    void Run();

private:
    Window* window = nullptr;
};
