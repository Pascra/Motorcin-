#pragma once
#include <string>
#include <vector>
#include <memory>

// Forward declarations
class Component;
class Transform;
class MeshRenderer;
class CameraComponent;

class GameObject {
public:
    GameObject(const std::string& name = "GameObject");
    ~GameObject();

    // Hierarchy
    void SetParent(GameObject* parent);
    GameObject* GetParent() const { return mParent; }
    void AddChild(GameObject* child);
    void RemoveChild(GameObject* child);
    const std::vector<GameObject*>& GetChildren() const { return mChildren; }

    // Transform (always present)
    Transform* GetTransform() const { return mTransform; }

    // Components
    template<typename T>
    T* AddComponent();

    template<typename T>
    T* GetComponent() const;

    template<typename T>
    bool HasComponent() const;

    void RemoveComponent(Component* comp);
    const std::vector<Component*>& GetComponents() const { return mComponents; }

    // Properties
    void SetName(const std::string& name) { mName = name; }
    const std::string& GetName() const { return mName; }

    void SetActive(bool active) { mActive = active; }
    bool IsActive() const { return mActive; }

    unsigned int GetID() const { return mID; }

    // Update
    void Update(float deltaTime);
    void Draw(class Camera* camera);

private:
    static unsigned int sNextID;
    unsigned int mID;
    std::string mName;
    bool mActive = true;

    GameObject* mParent = nullptr;
    std::vector<GameObject*> mChildren;

    Transform* mTransform = nullptr;
    std::vector<Component*> mComponents;
};

// Component base class
class Component {
public:
    Component(GameObject* owner) : mOwner(owner) {}
    virtual ~Component() = default;

    GameObject* GetOwner() const { return mOwner; }
    virtual const char* GetTypeName() const = 0;
    virtual void OnInspectorGUI() = 0;
    virtual void Update(float deltaTime) {}

protected:
    GameObject* mOwner;
};

// Transform Component (always present)
class Transform : public Component {
public:
    Transform(GameObject* owner);

    const char* GetTypeName() const override { return "Transform"; }
    void OnInspectorGUI() override;

    // Local transform
    void SetPosition(float x, float y, float z);
    void SetRotation(float x, float y, float z); // Euler angles
    void SetScale(float x, float y, float z);

    void GetPosition(float& x, float& y, float& z) const;
    void GetRotation(float& x, float& y, float& z) const;
    void GetScale(float& x, float& y, float& z) const;

    // World transform (considering hierarchy)
    void GetWorldPosition(float& x, float& y, float& z) const;
    void GetWorldMatrix(float out[16]) const;

private:
    float mPosition[3] = { 0, 0, 0 };
    float mRotation[3] = { 0, 0, 0 }; // Euler angles in degrees
    float mScale[3] = { 1, 1, 1 };
};

// MeshRenderer Component
class MeshRenderer : public Component {
public:
    MeshRenderer(GameObject* owner);
    ~MeshRenderer();

    const char* GetTypeName() const override { return "MeshRenderer"; }
    void OnInspectorGUI() override;

    void SetMesh(int meshIndex);
    void SetTexture(class Texture* texture);
    void SetColor(float r, float g, float b);

    int GetMeshIndex() const { return mMeshIndex; }
    class Texture* GetTexture() const { return mTexture; }
    void GetColor(float& r, float& g, float& b) const;

    void Draw(class Camera* camera, const float worldMatrix[16]);

private:
    int mMeshIndex = -1;
    class Texture* mTexture = nullptr;
    float mColor[3] = { 0.8f, 0.8f, 0.8f };
};

// Camera Component
class CameraComponent : public Component {
public:
    CameraComponent(GameObject* owner);

    const char* GetTypeName() const override { return "Camera"; }
    void OnInspectorGUI() override;

    void SetFOV(float fov) { mFOV = fov; }
    void SetNearPlane(float near) { mNear = near; }
    void SetFarPlane(float far) { mFar = far; }

    float GetFOV() const { return mFOV; }
    float GetNearPlane() const { return mNear; }
    float GetFarPlane() const { return mFar; }

    void GetProjectionMatrix(float out[16], float aspect) const;
    void GetViewMatrix(float out[16]) const;

private:
    float mFOV = 45.0f;
    float mNear = 0.1f;
    float mFar = 1000.0f;
};

// Template implementations
template<typename T>
T* GameObject::AddComponent() {
    T* comp = new T(this);
    mComponents.push_back(comp);
    return comp;
}

template<typename T>
T* GameObject::GetComponent() const {
    for (Component* comp : mComponents) {
        T* result = dynamic_cast<T*>(comp);
        if (result) return result;
    }
    return nullptr;
}

template<typename T>
bool GameObject::HasComponent() const {
    return GetComponent<T>() != nullptr;
}