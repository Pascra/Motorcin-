#pragma once
#include <SDL3/SDL.h>
#include <string>

class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    bool ShouldClose() const { return shouldClose || !valid_; }

    // ⬇⬇⬇ AÑADE ESTO
    bool IsValid() const { return valid_; }
    // ⬆⬆⬆

    void PollEvents();
    void SwapBuffers();

private:
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
    bool          shouldClose = false;
    bool          valid_ = false;
};
