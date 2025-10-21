#include "Application.h"

Application::Application() {
    window = new Window("Motorcin", 800, 600);
}

Application::~Application() {
    delete window;
}

void Application::Run() {
    while (!window->ShouldClose()) {
        window->PollEvents();

        Renderer::Clear(0.2f, 0.3f, 0.3f, 1.0f);

        window->SwapBuffers();
    }
}
