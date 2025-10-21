#include "Window.h"
#include <glad/glad.h>
#include <iostream>

Window::Window(const std::string& title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL Init failed: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL);
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

    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return;
    }

    glViewport(0, 0, width, height);
}

void Window::PollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT)
            shouldClose = true;
        else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
            shouldClose = true;
        else if (event.type == SDL_EVENT_WINDOW_RESIZED)
            glViewport(0, 0, event.window.data1, event.window.data2);
    }
}

void Window::SwapBuffers() {
    SDL_GL_SwapWindow(window);
}

Window::~Window() {
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
