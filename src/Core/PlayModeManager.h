#pragma once
#include <vector>
#include <string>

class GameObject;

enum class PlayMode {
    STOPPED,
    PLAYING,
    PAUSED
};

// Estructura para guardar el estado de un GameObject
struct GameObjectState {
    unsigned int id;
    std::string name;
    bool active;

    // Transform
    float position[3];
    float rotation[3];
    float scale[3];

    // Parent ID (0 = no parent)
    unsigned int parentID;

    // Components (guardamos solo los datos esenciales)
    bool hasMeshRenderer;
    int meshIndex;
    float color[3];

    bool hasCameraComponent;
    float fov;
    float nearPlane;
    float farPlane;
};

class PlayModeManager {
public:
    static void Init();
    static void Shutdown();

    // Play mode control
    static void Play();
    static void Pause();
    static void Stop();

    static PlayMode GetPlayMode() { return sPlayMode; }
    static bool IsPlaying() { return sPlayMode == PlayMode::PLAYING; }
    static bool IsPaused() { return sPlayMode == PlayMode::PAUSED; }
    static bool IsStopped() { return sPlayMode == PlayMode::STOPPED; }

    // State management
    static void SaveInitialState();
    static void RestoreInitialState();

    static GameObjectState CaptureGameObjectState(GameObject* obj);

private:
    static PlayMode sPlayMode;
    static std::vector<GameObjectState> sInitialState;
    static bool sHasSavedState;

    static void RestoreGameObjectState(const GameObjectState& state);
};