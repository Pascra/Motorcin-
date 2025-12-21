#include "SceneManager.h"
#include "Rendering/Camera.h"
#include "Rendering/Texture.h"
#include <algorithm>
#include <iostream>

std::vector<GameObject*> SceneManager::sRootObjects;
std::vector<GameObject*> SceneManager::sAllObjects;
GameObject* SceneManager::sSelectedObject = nullptr;

std::vector<std::pair<std::string, int>> SceneManager::sMeshes;
std::vector<std::pair<std::string, Texture*>> SceneManager::sTextures;

void SceneManager::Init() {
    std::cout << "[SceneManager] Initialized" << std::endl;
}

void SceneManager::Shutdown() {
    // Delete all root objects (which will cascade delete children)
    for (GameObject* obj : sRootObjects) {
        delete obj;
    }
    sRootObjects.clear();
    sAllObjects.clear();
    sSelectedObject = nullptr;

    // Clear resources (don't delete textures, they're managed elsewhere)
    sMeshes.clear();
    sTextures.clear();
}

GameObject* SceneManager::CreateGameObject(const std::string& name) {
    GameObject* obj = new GameObject(name);
    sRootObjects.push_back(obj);
    sAllObjects.push_back(obj);

    std::cout << "[SceneManager] Created: " << name << " (ID: " << obj->GetID() << ")" << std::endl;
    return obj;
}

void SceneManager::DestroyGameObject(GameObject* obj) {
    if (!obj) return;

    // Remove from selection
    if (sSelectedObject == obj) {
        sSelectedObject = nullptr;
    }

    // Remove from root objects
    auto it = std::find(sRootObjects.begin(), sRootObjects.end(), obj);
    if (it != sRootObjects.end()) {
        sRootObjects.erase(it);
    }

    // Remove from all objects
    auto it2 = std::find(sAllObjects.begin(), sAllObjects.end(), obj);
    if (it2 != sAllObjects.end()) {
        sAllObjects.erase(it2);
    }

    std::cout << "[SceneManager] Destroyed: " << obj->GetName() << std::endl;
    delete obj;
}

GameObject* SceneManager::GetGameObjectByID(unsigned int id) {
    for (GameObject* obj : sAllObjects) {
        if (obj->GetID() == id) {
            return obj;
        }
    }
    return nullptr;
}

const std::vector<GameObject*>& SceneManager::GetRootObjects() {
    return sRootObjects;
}

void SceneManager::SetSelectedObject(GameObject* obj) {
    sSelectedObject = obj;
}

GameObject* SceneManager::GetSelectedObject() {
    return sSelectedObject;
}

void SceneManager::Update(float deltaTime) {
    for (GameObject* obj : sRootObjects) {
        obj->Update(deltaTime);
    }
}

void SceneManager::Draw(Camera* camera) {
    for (GameObject* obj : sRootObjects) {
        obj->Draw(camera);
    }
}

void SceneManager::RegisterMesh(const std::string& name, int index) {
    // Check if already registered
    for (auto& pair : sMeshes) {
        if (pair.first == name) {
            pair.second = index;
            return;
        }
    }
    sMeshes.push_back({ name, index });
    std::cout << "[SceneManager] Registered mesh: " << name << " (index " << index << ")" << std::endl;
}

void SceneManager::RegisterTexture(const std::string& name, Texture* texture) {
    // Check if already registered
    for (auto& pair : sTextures) {
        if (pair.first == name) {
            pair.second = texture;
            return;
        }
    }
    sTextures.push_back({ name, texture });
    std::cout << "[SceneManager] Registered texture: " << name << std::endl;
}

const std::vector<std::pair<std::string, int>>& SceneManager::GetMeshList() {
    return sMeshes;
}

const std::vector<std::pair<std::string, Texture*>>& SceneManager::GetTextureList() {
    return sTextures;
}

int SceneManager::FindMeshIndex(const std::string& name) {
    for (const auto& pair : sMeshes) {
        if (pair.first == name) {
            return pair.second;
        }
    }
    return -1;
}

Texture* SceneManager::FindTexture(const std::string& name) {
    for (const auto& pair : sTextures) {
        if (pair.first == name) {
            return pair.second;
        }
    }
    return nullptr;
}