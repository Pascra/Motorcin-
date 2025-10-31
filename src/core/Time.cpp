#include "Time.h"

Uint64 Time::sLastTicks = 0;
float Time::sDeltaTime = 0.0f;
float Time::sTime = 0.0f;

void Time::Init() {
    sLastTicks = SDL_GetTicks();
    sDeltaTime = 0.0f;
    sTime = 0.0f;
}

void Time::Update() {
    Uint64 current = SDL_GetTicks();
    sDeltaTime = (current - sLastTicks) / 1000.0f;
    sTime += sDeltaTime;
    sLastTicks = current;
}