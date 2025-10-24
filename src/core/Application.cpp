#include "Application.h"
#include <iostream>

Application::Application() {
    window = new Window("Motorcin", 800, 600);
}

Application::~Application() {
    delete window;
}

void Application::Run() {
    if (!window || window->ShouldClose()) return;

    if (!Renderer::Init()) {
        std::cerr << "Renderer::Init failed\n";
        return;
    }

    // Puedes alternar entre DrawTriangle() y DrawRectangleIndexed()
    bool drawRectangle = true;

    while (!window->ShouldClose()) {
        window->PollEvents();

        Renderer::Clear(0.2f, 0.3f, 0.3f, 1.0f);

        if (drawRectangle) {
            // true = wireframe; pon false para sólido
            Renderer::DrawRectangleIndexed(false);
        }
        else {
            Renderer::DrawTriangle();
        }

        window->SwapBuffers();
    }

    Renderer::Shutdown();
}
