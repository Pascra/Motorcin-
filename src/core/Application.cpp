#include "Application.h"
#include <iostream>

Application::Application() {
    window = new Window("Motorcin Engine", 800, 600);
}

Application::~Application() {
    delete window;
}

void Application::Run() {
    if (!window || !window->IsValid()) {
        std::cerr << "Window / SDL failed to initialize. Exiting.\n";
        return;
    }

    if (!Renderer::Init()) {
        std::cerr << "Renderer::Init failed\n";
        return;
    }

    std::cout << "\n=== Motorcin Engine ===\n";
    std::cout << "Drag & Drop an FBX file to load it!\n";
    std::cout << "Press ESC to exit\n\n";

    while (!window->ShouldClose()) {
        window->PollEvents();

        Renderer::Clear(0.1f, 0.1f, 0.15f, 1.0f);

        // IMPORTANTE: Siempre intenta dibujar el modelo
        Renderer::DrawLoadedModel();

        window->SwapBuffers();
    }

    Renderer::Shutdown();
    std::cout << "Engine closed cleanly\n";
}