#include "Application.h"
#include "Input.h"
#include "Time.h"
#include "Renderer.h"
#include <iostream>
#include <cmath>
#include <filesystem>

Application::Application() {
    window = new Window("Motorcin Engine", 1280, 720);
    camera = new Camera();

    camera->SetPosition(0, 2, 8);
    camera->LookAt(0, 0, 0);

    std::cout << "[APP] Camera initialized" << std::endl;
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

    // 🆕 AUTO-CARGAR BAKERHOUSE AL INICIO
    std::string bakerHousePath = "assets/BakerHouse.fbx";
    if (std::filesystem::exists(bakerHousePath)) {
        std::cout << "\n[APP] Auto-loading BakerHouse..." << std::endl;
        Renderer::LoadModelFromPath(bakerHousePath);
    }
    else {
        std::cerr << "[APP] WARNING: BakerHouse.fbx not found at " << bakerHousePath << std::endl;
    }

    std::cout << "\n=== Motorcin Engine ===\n";
    std::cout << "Controls:\n";
    std::cout << "  - Drag & Drop FBX files to load\n";
    std::cout << "  - Hold RIGHT MOUSE BUTTON + move mouse to rotate camera\n";
    std::cout << "  - W/A/S/D to move camera\n";
    std::cout << "  - Q/E to move up/down\n";
    std::cout << "  - Mouse wheel to zoom\n";
    std::cout << "  - F to focus on model center\n";
    std::cout << "  - TAB to toggle wireframe/textured mode\n";
    std::cout << "  - G to toggle debug mode (shows model center)\n";
    std::cout << "  - T to toggle test triangle (yellow)\n";
    std::cout << "  - C to toggle backface culling\n";
    std::cout << "  - ESC to exit\n\n";

    bool showTestTriangle = false; // Cambiado a false por defecto

    while (!window->ShouldClose()) {
        Time::Update();
        float deltaTime = Time::GetDeltaTime();

        Input::Update();
        window->PollEvents();

        // ZOOM con scroll
        float wheel = Input::GetMouseWheelDelta();
        if (wheel != 0.0f) {
            camera->Zoom(wheel);
        }

        // ROTACIÓN con clic derecho
        if (Input::IsCameraControlActive() && wheel == 0.0f) {
            int dx, dy;
            Input::GetMouseDelta(dx, dy);

            if (abs(dx) > 1 || abs(dy) > 1) {
                camera->Rotate(static_cast<float>(-dx), static_cast<float>(-dy));
            }
        }

        // MOVIMIENTO con WASD/QE
        float moveSpeed = 5.0f;

        if (Input::IsKeyDown(SDLK_W)) {
            camera->MoveForward(moveSpeed * deltaTime);
        }
        if (Input::IsKeyDown(SDLK_S)) {
            camera->MoveForward(-moveSpeed * deltaTime);
        }
        if (Input::IsKeyDown(SDLK_A)) {
            float px, py, pz;
            camera->GetPosition(px, py, pz);
            camera->SetPosition(px - moveSpeed * deltaTime, py, pz);
        }
        if (Input::IsKeyDown(SDLK_D)) {
            float px, py, pz;
            camera->GetPosition(px, py, pz);
            camera->SetPosition(px + moveSpeed * deltaTime, py, pz);
        }
        if (Input::IsKeyDown(SDLK_Q)) {
            float px, py, pz;
            camera->GetPosition(px, py, pz);
            camera->SetPosition(px, py + moveSpeed * deltaTime, pz);
        }
        if (Input::IsKeyDown(SDLK_E)) {
            float px, py, pz;
            camera->GetPosition(px, py, pz);
            camera->SetPosition(px, py - moveSpeed * deltaTime, pz);
        }

        // AUTO-FOCUS cuando se carga modelo nuevo
        if (!modelLoadedLastFrame && Renderer::HasLoadedModel()) {
            float cx, cy, cz;
            Renderer::GetModelCenter(cx, cy, cz);
            float size = Renderer::GetModelSize();

            camera->SetSceneSize(size);

            float diagonal = size * std::sqrt(3.0f);
            float fovRad = 45.0f * 3.14159f / 180.0f;
            float distance = (diagonal * 0.5f) / std::tan(fovRad * 0.5f);
            distance *= 1.5f; // Factor de distancia

            std::cout << "\n*** AUTO-FOCUSING ON MODEL ***" << std::endl;
            std::cout << "Model center: (" << cx << ", " << cy << ", " << cz << ")" << std::endl;
            std::cout << "Model size: " << size << std::endl;
            std::cout << "Camera distance: " << distance << std::endl;

            camera->FocusOnPoint(cx, cy, cz, distance);

            float camX, camY, camZ;
            camera->GetPosition(camX, camY, camZ);
            std::cout << "Camera position: (" << camX << ", " << camY << ", " << camZ << ")" << std::endl;
        }

        modelLoadedLastFrame = Renderer::HasLoadedModel();

        // Tecla F para re-enfocar manualmente
        if (Input::IsKeyPressed(SDLK_F) && Renderer::HasLoadedModel()) {
            float cx, cy, cz;
            Renderer::GetModelCenter(cx, cy, cz);
            float size = Renderer::GetModelSize();

            camera->SetSceneSize(size);

            float diagonal = size * std::sqrt(3.0f);
            float fovRad = 45.0f * 3.14159f / 180.0f;
            float distance = (diagonal * 0.5f) / std::tan(fovRad * 0.5f);
            distance *= 1.5f;

            camera->FocusOnPoint(cx, cy, cz, distance);
        }

        // Tecla TAB para toggle wireframe
        if (Input::IsKeyPressed(SDLK_TAB)) {
            Renderer::ToggleWireframe();
        }

        // Tecla G para toggle debug mode
        if (Input::IsKeyPressed(SDLK_G)) {
            Renderer::SetDebugMode(!Renderer::IsDebugMode());
            std::cout << "Debug mode: " << (Renderer::IsDebugMode() ? "ON" : "OFF") << std::endl;
        }

        // Tecla T para toggle triángulo de test
        if (Input::IsKeyPressed(SDLK_T)) {
            showTestTriangle = !showTestTriangle;
            std::cout << "Test triangle: " << (showTestTriangle ? "ON" : "OFF") << std::endl;
        }

        // Tecla C para toggle culling
        if (Input::IsKeyPressed(SDLK_C)) {
            Renderer::ToggleCulling();
        }

        // Actualizar cámara
        camera->Update(deltaTime);

        // Clear antes de dibujar
        Renderer::Clear(0.1f, 0.1f, 0.15f, 1.0f);

        // Dibujar triángulo de test (si está activado)
        if (showTestTriangle) {
            Renderer::DrawDebugTriangle3D(camera);
        }

        // Dibujar modelo
        Renderer::DrawLoadedModel(camera);

        // Dibujar cubo de debug en el centro del modelo
        if (Renderer::IsDebugMode() && Renderer::HasLoadedModel()) {
            float cx, cy, cz;
            Renderer::GetModelCenter(cx, cy, cz);
            float size = Renderer::GetModelSize();
            Renderer::DrawDebugCube(camera, cx, cy, cz, size * 0.1f);
        }

        window->SwapBuffers();
    }

    Renderer::Shutdown();
    std::cout << "Engine closed cleanly\n";
}