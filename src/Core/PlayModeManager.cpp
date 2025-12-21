#include "PlayModeManager.h"
#include "SceneManager.h"
#include "GameObject.h"
#include <iostream>
#include <algorithm>
#include <functional>
#include <vector>

// Forward declaration
void SaveObjectAndChildren(GameObject* obj, std::vector<GameObjectState>& states);

PlayMode PlayModeManager::sPlayMode = PlayMode::STOPPED;
std::vector<GameObjectState> PlayModeManager::sInitialState;
bool PlayModeManager::sHasSavedState = false;

void PlayModeManager::Init() {
    sPlayMode = PlayMode::STOPPED;
    sInitialState.clear();
    sHasSavedState = false;
    std::cout << "[PlayModeManager] Initialized" << std::endl;
}

void PlayModeManager::Shutdown() {
    sInitialState.clear();
    sHasSavedState = false;
}

void PlayModeManager::Play() {
    if (sPlayMode == PlayMode::PLAYING) {
        return; // Ya está en play
    }

    if (sPlayMode == PlayMode::STOPPED) {
        // Guardar estado inicial antes de empezar
        std::cout << "[PlayMode] Saving initial state..." << std::endl;
        SaveInitialState();
        std::cout << "[PlayMode] PLAY - Simulation started" << std::endl;
    }
    else if (sPlayMode == PlayMode::PAUSED) {
        std::cout << "[PlayMode] RESUME - Simulation resumed" << std::endl;
    }

    sPlayMode = PlayMode::PLAYING;
}

void PlayModeManager::Pause() {
    if (sPlayMode != PlayMode::PLAYING) {
        return; // Solo se puede pausar si está jugando
    }

    std::cout << "[PlayMode] PAUSE - Simulation paused" << std::endl;
    sPlayMode = PlayMode::PAUSED;
}

void PlayModeManager::Stop() {
    if (sPlayMode == PlayMode::STOPPED) {
        return; // Ya está detenido
    }

    std::cout << "[PlayMode] STOP - Restoring initial state..." << std::endl;
    RestoreInitialState();
    std::cout << "[PlayMode] Simulation stopped" << std::endl;

    sPlayMode = PlayMode::STOPPED;
}

GameObjectState PlayModeManager::CaptureGameObjectState(GameObject* obj) {
    GameObjectState state;

    state.id = obj->GetID();
    state.name = obj->GetName();
    state.active = obj->IsActive();

    // Transform
    Transform* transform = obj->GetTransform();
    transform->GetPosition(state.position[0], state.position[1], state.position[2]);
    transform->GetRotation(state.rotation[0], state.rotation[1], state.rotation[2]);
    transform->GetScale(state.scale[0], state.scale[1], state.scale[2]);

    // Parent
    GameObject* parent = obj->GetParent();
    state.parentID = parent ? parent->GetID() : 0;

    // MeshRenderer
    MeshRenderer* meshRenderer = obj->GetComponent<MeshRenderer>();
    state.hasMeshRenderer = (meshRenderer != nullptr);
    if (state.hasMeshRenderer) {
        state.meshIndex = meshRenderer->GetMeshIndex();
        meshRenderer->GetColor(state.color[0], state.color[1], state.color[2]);
    }

    // CameraComponent
    CameraComponent* camera = obj->GetComponent<CameraComponent>();
    state.hasCameraComponent = (camera != nullptr);
    if (state.hasCameraComponent) {
        state.fov = camera->GetFOV();
        state.nearPlane = camera->GetNearPlane();
        state.farPlane = camera->GetFarPlane();
    }

    return state;
}

void SaveObjectAndChildren(GameObject* obj, std::vector<GameObjectState>& states) {
    states.push_back(PlayModeManager::CaptureGameObjectState(obj));

    for (GameObject* child : obj->GetChildren()) {
        SaveObjectAndChildren(child, states);
    }
}

void PlayModeManager::SaveInitialState() {
    sInitialState.clear();

    // Guardar todos los objetos raíz y sus hijos
    for (GameObject* root : SceneManager::GetRootObjects()) {
        SaveObjectAndChildren(root, sInitialState);
    }

    sHasSavedState = true;
    std::cout << "[PlayMode] Saved state of " << sInitialState.size() << " GameObjects" << std::endl;
}

void PlayModeManager::RestoreGameObjectState(const GameObjectState& state) {
    GameObject* obj = SceneManager::GetGameObjectByID(state.id);
    if (!obj) {
        std::cerr << "[PlayMode] WARNING: GameObject with ID " << state.id << " not found!" << std::endl;
        return;
    }

    // Restore basic properties
    obj->SetName(state.name);
    obj->SetActive(state.active);

    // Restore transform
    Transform* transform = obj->GetTransform();
    transform->SetPosition(state.position[0], state.position[1], state.position[2]);
    transform->SetRotation(state.rotation[0], state.rotation[1], state.rotation[2]);
    transform->SetScale(state.scale[0], state.scale[1], state.scale[2]);

    // Restore parent relationship
    GameObject* parent = nullptr;
    if (state.parentID != 0) {
        parent = SceneManager::GetGameObjectByID(state.parentID);
    }
    obj->SetParent(parent);

    // Restore MeshRenderer
    MeshRenderer* meshRenderer = obj->GetComponent<MeshRenderer>();
    if (state.hasMeshRenderer && meshRenderer) {
        meshRenderer->SetMesh(state.meshIndex);
        meshRenderer->SetColor(state.color[0], state.color[1], state.color[2]);
    }

    // Restore CameraComponent
    CameraComponent* camera = obj->GetComponent<CameraComponent>();
    if (state.hasCameraComponent && camera) {
        camera->SetFOV(state.fov);
        camera->SetNearPlane(state.nearPlane);
        camera->SetFarPlane(state.farPlane);
    }
}

void PlayModeManager::RestoreInitialState() {
    if (!sHasSavedState || sInitialState.empty()) {
        std::cout << "[PlayMode] No saved state to restore" << std::endl;
        return;
    }

    std::cout << "[PlayMode] Restoring " << sInitialState.size() << " GameObjects..." << std::endl;

    // Primero restaurar todos los objetos (sin jerarquía)
    for (const auto& state : sInitialState) {
        RestoreGameObjectState(state);
    }

    std::cout << "[PlayMode] State restored successfully" << std::endl;
}