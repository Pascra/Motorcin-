#include "Window.h"
#include <glad/glad.h>
#include <iostream>

Window::Window(const std::string& title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
        return;
    }

    // Atributos de contexto OpenGL 3.3 Core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // Doble buffer
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // Profundidad y stencil si se necesitan más adelante
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    if (SDL_GL_MakeCurrent(window, glContext) < 0) {
        std::cerr << "SDL_GL_MakeCurrent failed: " << SDL_GetError() << std::endl;
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    // VSync
    SDL_GL_SetSwapInterval(1);

    // Cargar punteros de OpenGL con GLAD usando el loader de SDL
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return;
    }

    glViewport(0, 0, width, height);
}

void Window::PollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            shouldClose = true;
        }
        else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
            shouldClose = true;
        }
        else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            glViewport(0, 0, event.window.data1, event.window.data2);
        }
    }
}

void Window::SwapBuffers() {
    SDL_GL_SwapWindow(window);
}

Window::~Window() {
    if (glContext) SDL_GL_DestroyContext(glContext);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}
