#include "GameObject.h"
#include "Rendering/Texture.h"
#include "Rendering/Camera.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <imgui.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// GameObject
// ============================================================

unsigned int GameObject::sNextID = 0;

GameObject::GameObject(const std::string& name)
    : mID(sNextID++)
    , mName(name)
{
    mTransform = new Transform(this);
}

GameObject::~GameObject() {
    // Remove from parent
    if (mParent) {
        mParent->RemoveChild(this);
    }

    // Delete children
    for (GameObject* child : mChildren) {
        child->mParent = nullptr;
        delete child;
    }
    mChildren.clear();

    // Delete components
    delete mTransform;
    for (Component* comp : mComponents) {
        delete comp;
    }
    mComponents.clear();
}

void GameObject::SetParent(GameObject* parent) {
    if (mParent == parent) return;

    // Remove from old parent
    if (mParent) {
        mParent->RemoveChild(this);
    }

    mParent = parent;

    // Add to new parent
    if (mParent) {
        mParent->AddChild(this);
    }
}

void GameObject::AddChild(GameObject* child) {
    if (!child) return;

    // Avoid duplicates
    auto it = std::find(mChildren.begin(), mChildren.end(), child);
    if (it == mChildren.end()) {
        mChildren.push_back(child);
    }
}

void GameObject::RemoveChild(GameObject* child) {
    auto it = std::find(mChildren.begin(), mChildren.end(), child);
    if (it != mChildren.end()) {
        mChildren.erase(it);
    }
}

void GameObject::RemoveComponent(Component* comp) {
    auto it = std::find(mComponents.begin(), mComponents.end(), comp);
    if (it != mComponents.end()) {
        delete* it;
        mComponents.erase(it);
    }
}

void GameObject::Update(float deltaTime) {
    if (!mActive) return;

    for (Component* comp : mComponents) {
        comp->Update(deltaTime);
    }

    for (GameObject* child : mChildren) {
        child->Update(deltaTime);
    }
}

void GameObject::Draw(Camera* camera) {
    if (!mActive) return;

    float worldMatrix[16];
    mTransform->GetWorldMatrix(worldMatrix);

    MeshRenderer* renderer = GetComponent<MeshRenderer>();
    if (renderer) {
        renderer->Draw(camera, worldMatrix);
    }

    for (GameObject* child : mChildren) {
        child->Draw(camera);
    }
}

// ============================================================
// Transform
// ============================================================

Transform::Transform(GameObject* owner)
    : Component(owner)
{
}

void Transform::OnInspectorGUI() {
    ImGui::Text("Position");
    ImGui::DragFloat3("##pos", mPosition, 0.1f);

    ImGui::Text("Rotation");
    ImGui::DragFloat3("##rot", mRotation, 1.0f);

    ImGui::Text("Scale");
    ImGui::DragFloat3("##scale", mScale, 0.01f);
}

void Transform::SetPosition(float x, float y, float z) {
    mPosition[0] = x;
    mPosition[1] = y;
    mPosition[2] = z;
}

void Transform::SetRotation(float x, float y, float z) {
    mRotation[0] = x;
    mRotation[1] = y;
    mRotation[2] = z;
}

void Transform::SetScale(float x, float y, float z) {
    mScale[0] = x;
    mScale[1] = y;
    mScale[2] = z;
}

void Transform::GetPosition(float& x, float& y, float& z) const {
    x = mPosition[0];
    y = mPosition[1];
    z = mPosition[2];
}

void Transform::GetRotation(float& x, float& y, float& z) const {
    x = mRotation[0];
    y = mRotation[1];
    z = mRotation[2];
}

void Transform::GetScale(float& x, float& y, float& z) const {
    x = mScale[0];
    y = mScale[1];
    z = mScale[2];
}

void Transform::GetWorldPosition(float& x, float& y, float& z) const {
    float matrix[16];
    GetWorldMatrix(matrix);
    x = matrix[12];
    y = matrix[13];
    z = matrix[14];
}

static void MatIdentity(float m[16]) {
    for (int i = 0; i < 16; ++i) m[i] = 0.f;
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

static void MatMul(float o[16], const float a[16], const float b[16]) {
    float r[16];
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            r[col + row * 4] = a[0 + row * 4] * b[col + 0 * 4]
            + a[1 + row * 4] * b[col + 1 * 4]
            + a[2 + row * 4] * b[col + 2 * 4]
            + a[3 + row * 4] * b[col + 3 * 4];
    for (int i = 0; i < 16; ++i) o[i] = r[i];
}

void Transform::GetWorldMatrix(float out[16]) const {
    // Create local TRS matrix
    float rotX = mRotation[0] * (float)M_PI / 180.0f;
    float rotY = mRotation[1] * (float)M_PI / 180.0f;
    float rotZ = mRotation[2] * (float)M_PI / 180.0f;

    float cx = std::cos(rotX), sx = std::sin(rotX);
    float cy = std::cos(rotY), sy = std::sin(rotY);
    float cz = std::cos(rotZ), sz = std::sin(rotZ);

    // Combined rotation matrix (ZYX order)
    float local[16];
    MatIdentity(local);

    local[0] = cy * cz;
    local[1] = cy * sz;
    local[2] = -sy;

    local[4] = sx * sy * cz - cx * sz;
    local[5] = sx * sy * sz + cx * cz;
    local[6] = sx * cy;

    local[8] = cx * sy * cz + sx * sz;
    local[9] = cx * sy * sz - sx * cz;
    local[10] = cx * cy;

    // Apply scale
    local[0] *= mScale[0]; local[4] *= mScale[0]; local[8] *= mScale[0];
    local[1] *= mScale[1]; local[5] *= mScale[1]; local[9] *= mScale[1];
    local[2] *= mScale[2]; local[6] *= mScale[2]; local[10] *= mScale[2];

    // Apply translation
    local[12] = mPosition[0];
    local[13] = mPosition[1];
    local[14] = mPosition[2];

    // If has parent, multiply by parent's world matrix
    if (mOwner->GetParent()) {
        float parentMatrix[16];
        mOwner->GetParent()->GetTransform()->GetWorldMatrix(parentMatrix);
        MatMul(out, parentMatrix, local);
    }
    else {
        for (int i = 0; i < 16; ++i) out[i] = local[i];
    }
}

// ============================================================
// MeshRenderer
// ============================================================

MeshRenderer::MeshRenderer(GameObject* owner)
    : Component(owner)
{
}

MeshRenderer::~MeshRenderer() {
    // Note: We don't delete the texture here as it's managed by ResourceManager
}

void MeshRenderer::OnInspectorGUI() {
    ImGui::Text("Mesh Index: %d", mMeshIndex);

    // Drag & Drop target for meshes
    ImGui::Button("Drop Mesh Here", ImVec2(-1, 30));
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MESH")) {
            int* meshIndex = (int*)payload->Data;
            mMeshIndex = *meshIndex;
            std::cout << "Assigned mesh index: " << mMeshIndex << std::endl;
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Text("Color");
    ImGui::ColorEdit3("##color", mColor);

    ImGui::Text("Texture: %s", mTexture ? "Assigned" : "None");

    // Drag & Drop target for textures
    ImGui::Button("Drop Texture Here", ImVec2(-1, 30));
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE")) {
            Texture** texPtr = (Texture**)payload->Data;
            mTexture = *texPtr;
            std::cout << "Assigned texture" << std::endl;
        }
        ImGui::EndDragDropTarget();
    }

    if (mTexture && ImGui::Button("Clear Texture")) {
        mTexture = nullptr;
    }
}

void MeshRenderer::SetMesh(int meshIndex) {
    mMeshIndex = meshIndex;
}

void MeshRenderer::SetTexture(Texture* texture) {
    mTexture = texture;
}

void MeshRenderer::SetColor(float r, float g, float b) {
    mColor[0] = r;
    mColor[1] = g;
    mColor[2] = b;
}

void MeshRenderer::GetColor(float& r, float& g, float& b) const {
    r = mColor[0];
    g = mColor[1];
    b = mColor[2];
}

void MeshRenderer::Draw(Camera* camera, const float worldMatrix[16]) {
    // This will be called by the Renderer system
    // For now, it's a placeholder
}

// ============================================================
// CameraComponent
// ============================================================

CameraComponent::CameraComponent(GameObject* owner)
    : Component(owner)
{
}

void CameraComponent::OnInspectorGUI() {
    ImGui::Text("Field of View");
    ImGui::SliderFloat("##fov", &mFOV, 10.0f, 120.0f);

    ImGui::Text("Near Plane");
    ImGui::DragFloat("##near", &mNear, 0.01f, 0.01f, 10.0f);

    ImGui::Text("Far Plane");
    ImGui::DragFloat("##far", &mFar, 1.0f, 10.0f, 10000.0f);
}

static void MatPerspective(float m[16], float fovy, float aspect, float zn, float zf) {
    if (aspect <= 0.0f) aspect = 1.0f;
    if (zn <= 0.0f) zn = 0.01f;
    if (zf <= zn) zf = zn + 1000.0f;

    const float f = 1.0f / std::tan(fovy * 0.5f);

    for (int i = 0; i < 16; ++i) m[i] = 0.f;

    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zf + zn) / (zn - zf);
    m[11] = -1.f;
    m[14] = (2.f * zf * zn) / (zn - zf);
    m[15] = 0.f;
}

void CameraComponent::GetProjectionMatrix(float out[16], float aspect) const {
    MatPerspective(out, mFOV * (float)M_PI / 180.0f, aspect, mNear, mFar);
}

void CameraComponent::GetViewMatrix(float out[16]) const {
    float worldMatrix[16];
    mOwner->GetTransform()->GetWorldMatrix(worldMatrix);

    // Invert world matrix to get view matrix
    // This is a simplified version - for production use proper matrix inversion
    float px, py, pz;
    mOwner->GetTransform()->GetWorldPosition(px, py, pz);

    // For now, use identity and just translate
    MatIdentity(out);
    out[12] = -px;
    out[13] = -py;
    out[14] = -pz;
}