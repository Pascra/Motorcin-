#pragma once
#include <SDL3/SDL.h>
#include <string>

class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    bool ShouldClose() const { return shouldClose || !valid_; }
    bool IsValid() const { return valid_; }

    void PollEvents();
    void SwapBuffers();

    // For ImGui
    SDL_Window* GetSDLWindow() const { return window; }
    SDL_GLContext GetGLContext() const { return glContext; }

private:
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
    bool shouldClose = false;
    bool valid_ = false;
};