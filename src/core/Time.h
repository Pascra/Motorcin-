#pragma once
#include <SDL3/SDL.h>

class Time {
public:
    static void Init();
    static void Update();
    static float GetDeltaTime() { return sDeltaTime; }
    static float GetTime() { return sTime; }

private:
    static Uint64 sLastTicks;
    static float sDeltaTime;
    static float sTime;
};