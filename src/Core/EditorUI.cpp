// EditorUI.cpp (DEFINITIVO) — con Project Assets + Delete (PASO 5.2 incluido)

#include "EditorUI.h"
#include "SceneManager.h"
#include "GameObject.h"
#include "PlayModeManager.h"
#include "Rendering/Texture.h"
#include "AssetDatabase.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <cstring>
#include <filesystem>

// Static member initialization
bool EditorUI::sShowHierarchy = true;
bool EditorUI::sShowInspector = true;
bool EditorUI::sShowResources = true;
bool EditorUI::sShouldExit = false;
GameObject* EditorUI::sDraggedObject = nullptr;
std::string EditorUI::sNewObjectName = "New GameObject";
static std::string sSelectedAssetGuid;
static bool sPendingDeleteAsset = false;
static bool sPendingForceDeleteAsset = false;
static std::string sPendingDeleteGuid;
void EditorUI::Init() {
    std::cout << "[EditorUI] Initialized" << std::endl;
}

void EditorUI::Shutdown() {
    std::cout << "[EditorUI] Shutdown" << std::endl;
}

bool EditorUI::ShouldExit() {
    return sShouldExit;
}

void EditorUI::BeginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void EditorUI::EndFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorUI::DrawMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene")) {
                // Confirm dialog
                if (ImGui::BeginPopupModal("Confirm New Scene")) {
                    ImGui::Text("Are you sure? This will clear the current scene.");
                    if (ImGui::Button("Yes")) {
                        SceneManager::Shutdown();
                        SceneManager::Init();
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("No")) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                else {
                    ImGui::OpenPopup("Confirm New Scene");
                }
            }
            if (ImGui::MenuItem("Exit")) {
                sShouldExit = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject")) {
            if (ImGui::MenuItem("Create Empty")) {
                GameObject* obj = SceneManager::CreateGameObject("Empty GameObject");
                SceneManager::SetSelectedObject(obj);
            }

            if (ImGui::MenuItem("Create Cube")) {
                GameObject* obj = SceneManager::CreateGameObject("Cube");
                MeshRenderer* renderer = obj->AddComponent<MeshRenderer>();
                renderer->SetMesh(0);
                SceneManager::SetSelectedObject(obj);
            }

            if (ImGui::MenuItem("Create Camera")) {
                GameObject* obj = SceneManager::CreateGameObject("Camera");
                obj->AddComponent<CameraComponent>();
                SceneManager::SetSelectedObject(obj);
            }

            ImGui::Separator();

            GameObject* selected = SceneManager::GetSelectedObject();
            if (selected) {
                if (ImGui::MenuItem("Delete Selected", "DEL")) {
                    SceneManager::DestroyGameObject(selected);
                }
                if (ImGui::MenuItem("Duplicate Selected", "Ctrl+D")) {
                    std::cout << "Duplicate not yet implemented" << std::endl;
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Component")) {
            GameObject* selected = SceneManager::GetSelectedObject();
            if (selected) {
                if (ImGui::MenuItem("Add MeshRenderer") && !selected->HasComponent<MeshRenderer>()) {
                    selected->AddComponent<MeshRenderer>();
                }
                if (ImGui::MenuItem("Add Camera") && !selected->HasComponent<CameraComponent>()) {
                    selected->AddComponent<CameraComponent>();
                }
            }
            else {
                ImGui::TextDisabled("(Select a GameObject first)");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Hierarchy", nullptr, &sShowHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &sShowInspector);
            ImGui::MenuItem("Resources", nullptr, &sShowResources);
            ImGui::EndMenu();
        }

        // Stats on the right
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 200);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::EndMainMenuBar();
    }
}

void EditorUI::DrawGameObjectNode(GameObject* obj) {
    if (!obj) return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (SceneManager::GetSelectedObject() == obj) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (obj->GetChildren().empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool nodeOpen = ImGui::TreeNodeEx(obj, flags, "%s", obj->GetName().c_str());

    // Selection
    if (ImGui::IsItemClicked()) {
        SceneManager::SetSelectedObject(obj);
    }

    // Context menu
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Create Child")) {
            GameObject* child = SceneManager::CreateGameObject("Child");
            child->SetParent(obj);
        }

        if (ImGui::MenuItem("Delete")) {
            SceneManager::DestroyGameObject(obj);
            ImGui::EndPopup();
            if (nodeOpen) ImGui::TreePop();
            return;
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Rename")) {
            // TODO: Open rename dialog
        }

        ImGui::EndPopup();
    }

    // Drag source
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("GAMEOBJECT", &obj, sizeof(GameObject*));
        ImGui::Text("Move %s", obj->GetName().c_str());
        sDraggedObject = obj;
        ImGui::EndDragDropSource();
    }

    // Drop target (reparent)
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT")) {
            GameObject* draggedObj = *(GameObject**)payload->Data;
            if (draggedObj != obj && draggedObj->GetParent() != obj) {
                bool isDescendant = false;
                GameObject* check = obj;
                while (check) {
                    if (check == draggedObj) {
                        isDescendant = true;
                        break;
                    }
                    check = check->GetParent();
                }

                if (!isDescendant) {
                    draggedObj->SetParent(obj);
                    std::cout << "Reparented " << draggedObj->GetName()
                        << " to " << obj->GetName() << std::endl;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Draw children
    if (nodeOpen) {
        for (GameObject* child : obj->GetChildren()) {
            DrawGameObjectNode(child);
        }
        ImGui::TreePop();
    }
}

void EditorUI::DrawHierarchy() {
    if (!sShowHierarchy) return;

    ImGui::Begin("Hierarchy", &sShowHierarchy);

    if (ImGui::Button("+ Create GameObject", ImVec2(-1, 0))) {
        GameObject* obj = SceneManager::CreateGameObject(sNewObjectName);
        SceneManager::SetSelectedObject(obj);
    }

    ImGui::Separator();

    GameObject* selected = SceneManager::GetSelectedObject();
    if (selected && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        SceneManager::DestroyGameObject(selected);
        selected = nullptr;
    }

    for (GameObject* obj : SceneManager::GetRootObjects()) {
        DrawGameObjectNode(obj);
    }

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered()) {
        SceneManager::SetSelectedObject(nullptr);
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT")) {
            GameObject* draggedObj = *(GameObject**)payload->Data;
            draggedObj->SetParent(nullptr);
            std::cout << "Made " << draggedObj->GetName() << " a root object" << std::endl;
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void EditorUI::DrawComponentInspector(GameObject* obj) {
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        obj->GetTransform()->OnInspectorGUI();
    }

    for (Component* comp : obj->GetComponents()) {
        std::string header = std::string(comp->GetTypeName());

        if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            comp->OnInspectorGUI();

            ImGui::Spacing();
            if (ImGui::Button(("Remove##" + header).c_str())) {
                obj->RemoveComponent(comp);
                break;
            }
        }
    }
}

void EditorUI::DrawInspector() {
    if (!sShowInspector) return;

    ImGui::Begin("Inspector", &sShowInspector);

    GameObject* selected = SceneManager::GetSelectedObject();

    if (selected) {
        bool isPlaying = PlayModeManager::IsPlaying();
        if (isPlaying) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
            ImGui::TextWrapped("⚠ Playing: Changes will be lost when stopping");
            ImGui::PopStyleColor();
            ImGui::Separator();
        }

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Text("ID: %u", selected->GetID());
        ImGui::PopStyleColor();

        ImGui::Separator();

        char nameBuf[256];
        strncpy(nameBuf, selected->GetName().c_str(), sizeof(nameBuf) - 1);
        nameBuf[sizeof(nameBuf) - 1] = '\0';

        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf))) {
            selected->SetName(nameBuf);
        }
        ImGui::PopItemWidth();

        bool active = selected->IsActive();
        if (ImGui::Checkbox("Active", &active)) {
            selected->SetActive(active);
        }

        ImGui::Separator();
        ImGui::Spacing();

        DrawComponentInspector(selected);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = 200.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonWidth) * 0.5f);

        if (ImGui::Button("Add Component", ImVec2(buttonWidth, 30))) {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup")) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Add Component");
            ImGui::Separator();

            if (!selected->HasComponent<MeshRenderer>()) {
                if (ImGui::MenuItem("Mesh Renderer")) {
                    selected->AddComponent<MeshRenderer>();
                    ImGui::CloseCurrentPopup();
                }
            }

            if (!selected->HasComponent<CameraComponent>()) {
                if (ImGui::MenuItem("Camera")) {
                    selected->AddComponent<CameraComponent>();
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }

    }
    else {
        ImGui::TextDisabled("No GameObject selected");
        ImGui::Spacing();
        ImGui::TextWrapped("Select a GameObject from the Hierarchy to view and edit its properties.");
    }

    ImGui::End();
}

void EditorUI::DrawResourceBrowser() {
    if (!sShowResources) return;

    ImGui::Begin("Resources", &sShowResources);

    // ============================================================
    // ✅ PASO 5.2 — Project Assets (AssetDatabase) + Delete
    // ============================================================
    if (ImGui::CollapsingHeader("Project Assets", ImGuiTreeNodeFlags_DefaultOpen)) {

        // ⚠️ Estas estáticas deben vivir fuera del for (y el delete fuera del for)
        static bool sPendingDelete = false;
        static bool sPendingForceDelete = false;
        static std::string sPendingGuid;
        static std::string sSelectedGuid;

        const auto& all = AssetDatabase::GetAll();

        if (all.empty()) {
            ImGui::TextDisabled("  No assets imported yet.");
            ImGui::TextDisabled("  Tip: Drag & drop FBX/PNG into the window to import.");

            // si no hay assets, no puede haber selección válida
            sSelectedGuid.clear();
        }
        else {
            ImGui::Indent();

            for (const auto& kv : all) {
                const AssetRecord& rec = kv.second;

                // Seguridad por si llega un record raro
                if (rec.guid.empty()) continue;

                ImGui::PushID(rec.guid.c_str());

                const char* icon = "[A]";
                if (rec.type == AssetType::Model) icon = "[FBX]";
                else if (rec.type == AssetType::Texture) icon = "[TEX]";

                std::string filename = rec.sourcePath;
                try { filename = std::filesystem::path(rec.sourcePath).filename().string(); }
                catch (...) {}

                ImGui::Text("%s", icon);
                ImGui::SameLine();

                bool isSelected = (sSelectedGuid == rec.guid);
                if (ImGui::Selectable(filename.c_str(), isSelected)) {
                    sSelectedGuid = rec.guid;
                }

                // Tooltip
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Path: %s", rec.sourcePath.c_str());
                    ImGui::Text("GUID: %s", rec.guid.c_str());
                    ImGui::Text("Library: %s", rec.libraryDir.c_str());
                    ImGui::TextDisabled("Right click for options");
                    ImGui::EndTooltip();
                }

                // Context menu
                if (ImGui::BeginPopupContextItem()) {
                    ImGui::TextDisabled("GUID: %s", rec.guid.c_str());
                    ImGui::Separator();

                    if (ImGui::MenuItem("Delete")) {
                        ImGui::CloseCurrentPopup();          // ✅ cerrar popup
                        sPendingDelete = true;              // ✅ marcar delete
                        sPendingForceDelete = false;
                        sPendingGuid = rec.guid;
                    }

                    if (ImGui::MenuItem("Force Delete")) {
                        ImGui::CloseCurrentPopup();
                        sPendingDelete = true;
                        sPendingForceDelete = true;
                        sPendingGuid = rec.guid;
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            ImGui::Unindent();
        }

        // ✅ Ejecutar delete FUERA del bucle (clave para no crashear)
        if (sPendingDelete) {
            const std::string guidToDelete = sPendingGuid;   // copia local segura
            const bool force = sPendingForceDelete;

            sPendingDelete = false;
            sPendingForceDelete = false;
            sPendingGuid.clear();

            if (!guidToDelete.empty()) {
                std::cout << "[EditorUI] Request delete guid=" << guidToDelete << "\n";
                AssetDatabase::DeleteAsset(guidToDelete, force);

                // si borraste el seleccionado, limpia selección
                if (sSelectedGuid == guidToDelete) {
                    sSelectedGuid.clear();
                }
            }
        }
    }

    ImGui::End();
}

void EditorUI::HandleDragDrop() {
    GameObject* selected = SceneManager::GetSelectedObject();
    if (!selected) return;

    MeshRenderer* renderer = selected->GetComponent<MeshRenderer>();
    if (!renderer) return;

    // Placeholder
}

bool EditorUI::IsWindowHovered() {
    return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
}

void EditorUI::DrawPlayControls() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 150, 30), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, 60), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("PlayControls", nullptr, flags);

    PlayMode mode = PlayModeManager::GetPlayMode();

    // Play button
    if (mode == PlayMode::STOPPED || mode == PlayMode::PAUSED) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.5f, 0.1f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 0.5f));
    }

    if (ImGui::Button("▶ Play", ImVec2(90, 40))) {
        PlayModeManager::Play();
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Pause button
    bool canPause = (mode == PlayMode::PLAYING);
    if (!canPause) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.1f, 1.0f));
    }

    if (ImGui::Button("⏸ Pause", ImVec2(90, 40)) && canPause) {
        PlayModeManager::Pause();
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Stop button
    bool canStop = (mode != PlayMode::STOPPED);
    if (!canStop) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    }

    if (ImGui::Button("⏹ Stop", ImVec2(90, 40)) && canStop) {
        PlayModeManager::Stop();
    }
    ImGui::PopStyleColor(3);

    ImGui::Spacing();
    const char* statusText = "";
    ImVec4 statusColor;

    switch (mode) {
    case PlayMode::PLAYING:
        statusText = "▶ PLAYING";
        statusColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
        break;
    case PlayMode::PAUSED:
        statusText = "⏸ PAUSED";
        statusColor = ImVec4(1.0f, 1.0f, 0.2f, 1.0f);
        break;
    case PlayMode::STOPPED:
        statusText = "⏹ STOPPED";
        statusColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        break;
    }

    float textWidth = ImGui::CalcTextSize(statusText).x;
    ImGui::SetCursorPosX((300 - textWidth) * 0.5f);
    ImGui::TextColored(statusColor, "%s", statusText);

    ImGui::End();
}
