#include "Application.h"
#include "Input.h"
#include "Time.h"
#include "SceneManager.h"
#include "EditorUI.h"
#include "GameObject.h"
#include "Rendering/Renderer.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
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

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window->GetSDLWindow(), window->GetGLContext());
    ImGui_ImplOpenGL3_Init("#version 330");

    Input::Init();
    Time::Init();
    SceneManager::Init();
    EditorUI::Init();

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

    bool showTestTriangle = false;

    const char* DEFAULT_MODEL = "assets/Street/StreetEnvironment_v01.FBX";
    bool autoLoadPending = true;

    // Create initial scene
    GameObject* mainCamera = SceneManager::CreateGameObject("Main Camera");
    mainCamera->AddComponent<CameraComponent>();
    mainCamera->GetTransform()->SetPosition(0, 2, 8);

    while (!window->ShouldClose() && !EditorUI::ShouldExit()) {
        Time::Update();
        float deltaTime = Time::GetDeltaTime();

        if (autoLoadPending) {
            autoLoadPending = false;

            std::cout << "\n[APP] Looking for default model: " << DEFAULT_MODEL << std::endl;

            if (std::filesystem::exists(DEFAULT_MODEL)) {
                std::cout << "[APP] Found! Loading default model..." << std::endl;
                if (Renderer::LoadModelFromPath(DEFAULT_MODEL)) {
                    std::cout << "[APP] Model loaded successfully!" << std::endl;

                    // Register meshes in SceneManager
                    SceneManager::RegisterMesh("Street Environment", 0);
                }
                else {
                    std::cerr << "[APP] Failed to load model!" << std::endl;
                }
            }
            else {
                std::cerr << "[APP] Default model not found" << std::endl;
            }
        }

        Input::Update();
        window->PollEvents();

        // Check if ImGui wants to capture input
        ImGuiIO& io = ImGui::GetIO();
        bool imguiWantsMouse = io.WantCaptureMouse;
        bool imguiWantsKeyboard = io.WantCaptureKeyboard;

        // Only process camera input if ImGui is not using input
        if (!imguiWantsMouse && !imguiWantsKeyboard) {
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
        } // Fin del if (!imguiWantsMouse && !imguiWantsKeyboard)

        // AUTO-FOCUS cuando se carga modelo nuevo
        if (!modelLoadedLastFrame && Renderer::HasLoadedModel()) {
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

        // Update scene
        SceneManager::Update(deltaTime);

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

        // Draw scene GameObjects
        SceneManager::Draw(camera);

        // Draw UI
        EditorUI::BeginFrame();
        EditorUI::DrawMainMenuBar();
        EditorUI::DrawHierarchy();
        EditorUI::DrawInspector();
        EditorUI::DrawResourceBrowser();
        EditorUI::EndFrame();

        window->SwapBuffers();
    }

    // Cleanup
    EditorUI::Shutdown();
    SceneManager::Shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    Renderer::Shutdown();
    std::cout << "Engine closed cleanly\n";
}