#pragma once
#include "GameObject.h"
#include <vector>
#include <string>

class Texture;

class SceneManager {
public:
    static void Init();
    static void Shutdown();

    // GameObject management
    static GameObject* CreateGameObject(const std::string& name = "GameObject");
    static void DestroyGameObject(GameObject* obj);
    static GameObject* GetGameObjectByID(unsigned int id);

    // Root objects (objects without parent)
    static const std::vector<GameObject*>& GetRootObjects();

    // Selection
    static void SetSelectedObject(GameObject* obj);
    static GameObject* GetSelectedObject();

    // Update & Draw
    static void Update(float deltaTime);
    static void Draw(class Camera* camera);

    // Resource management
    static void RegisterMesh(const std::string& name, int index);
    static void RegisterTexture(const std::string& name, Texture* texture);

    static const std::vector<std::pair<std::string, int>>& GetMeshList();
    static const std::vector<std::pair<std::string, Texture*>>& GetTextureList();

    static int FindMeshIndex(const std::string& name);
    static Texture* FindTexture(const std::string& name);

private:
    static std::vector<GameObject*> sRootObjects;
    static std::vector<GameObject*> sAllObjects;
    static GameObject* sSelectedObject;

    static std::vector<std::pair<std::string, int>> sMeshes;
    static std::vector<std::pair<std::string, Texture*>> sTextures;
};