#include "Window.h"
#include "Renderer.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>
#include <string>

static void DumpSDLInfo()
{
    // Versión de compilación (macros en SDL3)
    std::cout << "SDL compiled version: "
        << (int)SDL_MAJOR_VERSION << "."
        << (int)SDL_MINOR_VERSION << "."
        << (int)SDL_MICRO_VERSION << "\n";

    // Drivers de vídeo disponibles (no requiere Init)
    const int n = SDL_GetNumVideoDrivers();
    std::cout << "Video drivers available (" << n << "): ";
    for (int i = 0; i < n; ++i) {
        if (i) std::cout << ", ";
        std::cout << SDL_GetVideoDriver(i);
    }
    std::cout << "\n";
}
Window::Window(const std::string& title, int width, int height)
{
    DumpSDLInfo();

    // 0 = OK ; <0 = error
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return;
    }

    // Forzar driver de vídeo si hiciera falta (comenta esta línea si no ayuda)
    // SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "windows");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    window = SDL_CreateWindow(title.c_str(), width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return;
    }

    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window); window = nullptr;
        SDL_Quit();
        return;
    }

    if (SDL_GL_MakeCurrent(window, glContext) != 0) {
        std::cerr << "SDL_GL_MakeCurrent failed: " << SDL_GetError() << "\n";
        SDL_GL_DestroyContext(glContext); glContext = nullptr;
        SDL_DestroyWindow(window); window = nullptr;
        SDL_Quit();
        return;
    }

    if (SDL_GL_SetSwapInterval(1) != 0) {
        std::cerr << "Warning: SDL_GL_SetSwapInterval failed: " << SDL_GetError() << "\n";
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        SDL_GL_DestroyContext(glContext); glContext = nullptr;
        SDL_DestroyWindow(window); window = nullptr;
        SDL_Quit();
        return;
    }

    glViewport(0, 0, width, height);
    valid_ = true;
    std::cout << "Window and OpenGL initialized successfully!\n";
}

void Window::PollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_EVENT_QUIT: shouldClose = true; break;
        case SDL_EVENT_KEY_DOWN:
            if (e.key.key == SDLK_ESCAPE) shouldClose = true;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            Renderer::SetViewportSize(e.window.data1, e.window.data2);
            break;
        case SDL_EVENT_DROP_FILE: {
            const char* cdata = e.drop.data;
            std::string path = cdata ? std::string(cdata) : std::string();
            if (cdata) SDL_free(const_cast<char*>(cdata)); // liberar DESPUÉS de copiar
            if (!path.empty()) Renderer::OnFileDropped(path.c_str());
            break;
        }
        default: break;
        }
    }
}

void Window::SwapBuffers() {
    if (window) SDL_GL_SwapWindow(window);
}

Window::~Window() {
    if (glContext) { SDL_GL_DestroyContext(glContext); glContext = nullptr; }
    if (window) { SDL_DestroyWindow(window);       window = nullptr; }
    SDL_Quit();
}
