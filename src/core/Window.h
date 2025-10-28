#pragma once
#include <string>
#include <SDL3/SDL.h>

class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    void PollEvents();
    void SwapBuffers();

    bool IsValid() const { return valid_; }
    bool ShouldClose() const { return shouldClose; }

private:
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
    bool valid_ = false;
    bool shouldClose = false;
};
