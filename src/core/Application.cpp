#include "Application.h"
#include "Input.h"
#include "Time.h"
#include <iostream>

Application::Application() {
    window = new Window("Motorcin Engine", 800, 600);
    camera = new Camera();
}

Application::~Application() {
    delete camera;
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

    Input::Init();
    Time::Init();

    std::cout << "\n=== Motorcin Engine ===\n";
    std::cout << "Controls:\n";
    std::cout << "  - Drag & Drop FBX files to load\n";
    std::cout << "  - Hold RIGHT MOUSE BUTTON + move mouse to rotate camera\n";
    std::cout << "  - W/A/S/D to move camera\n";
    std::cout << "  - Q/E to move up/down\n";
    std::cout << "  - Mouse wheel to zoom\n";
    std::cout << "  - ESC to exit\n\n";

    while (!window->ShouldClose()) {
        Time::Update();
        Input::Update();

        window->PollEvents();

        camera->Update(Time::GetDeltaTime());

        Renderer::Clear(0.1f, 0.1f, 0.15f, 1.0f);
        Renderer::DrawLoadedModel(camera);

        window->SwapBuffers();
    }

    Renderer::Shutdown();
    std::cout << "Engine closed cleanly\n";
}