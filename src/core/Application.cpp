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
    std::cout << "  - F to focus on model center\n";
    std::cout << "  - TAB to toggle wireframe/textured mode\n";  // NUEVO
    std::cout << "  - ESC to exit\n\n";

    int frameCount = 0;

    while (!window->ShouldClose()) {
        Time::Update();
        Input::Update();

        window->PollEvents();

        // AUTO-FOCUS cuando se carga un modelo nuevo
        if (!modelLoadedLastFrame && Renderer::HasLoadedModel()) {
            float cx, cy, cz;
            Renderer::GetModelCenter(cx, cy, cz);
            float size = Renderer::GetModelSize();

            // MEJOR CÁLCULO: distancia basada en FOV de 45 grados
            // Para que el modelo ocupe ~80% de la pantalla
            float distance = size / (2.0f * std::tan(45.0f * 3.14159f / 180.0f / 2.0f)) * 1.5f;

            // Asegurar distancia mínima
            if (distance < 3.0f) distance = 3.0f;

            std::cout << "\n*** CAMERA AUTO-FOCUSED ***" << std::endl;
            std::cout << "Model center: (" << cx << ", " << cy << ", " << cz << ")" << std::endl;
            std::cout << "Model size: " << size << std::endl;
            std::cout << "Camera distance: " << distance << std::endl;

            camera->FocusOnPoint(cx, cy, cz, distance);

            float camX, camY, camZ;
            camera->GetPosition(camX, camY, camZ);
            std::cout << "Camera position: (" << camX << ", " << camY << ", " << camZ << ")" << std::endl;
            std::cout << "Press F to re-focus anytime\n" << std::endl;
        }
        modelLoadedLastFrame = Renderer::HasLoadedModel();

        // Tecla F para re-enfocar manualmente
        if (Input::IsKeyPressed(SDLK_F) && Renderer::HasLoadedModel()) {
            float cx, cy, cz;
            Renderer::GetModelCenter(cx, cy, cz);
            float size = Renderer::GetModelSize();

            float distance = size / (2.0f * std::tan(45.0f * 3.14159f / 180.0f / 2.0f)) * 1.5f;
            if (distance < 3.0f) distance = 3.0f;

            camera->FocusOnPoint(cx, cy, cz, distance);

            float camX, camY, camZ;
            camera->GetPosition(camX, camY, camZ);
            std::cout << "Camera re-focused. Position: (" << camX << ", " << camY << ", " << camZ << ")\n";
        }

        // NUEVO: Tecla TAB para toggle wireframe
        if (Input::IsKeyPressed(SDLK_TAB)) {
            Renderer::ToggleWireframe();
        }

        camera->Update(Time::GetDeltaTime());

        Renderer::Clear(0.1f, 0.1f, 0.15f, 1.0f);
        Renderer::DrawLoadedModel(camera);

        window->SwapBuffers();
    }

    Renderer::Shutdown();
    std::cout << "Engine closed cleanly\n";
}