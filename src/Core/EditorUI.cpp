#include "EditorUI.h"
#include "SceneManager.h"
#include "GameObject.h"
#include "Rendering/Texture.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <cstring>

// Static member initialization
bool EditorUI::sShowHierarchy = true;
bool EditorUI::sShowInspector = true;
bool EditorUI::sShowResources = true;
bool EditorUI::sShouldExit = false;
GameObject* EditorUI::sDraggedObject = nullptr;
std::string EditorUI::sNewObjectName = "New GameObject";

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
                    // TODO: Implement duplication
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
                // Avoid circular dependency
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

    // Create new GameObject button
    if (ImGui::Button("+ Create GameObject", ImVec2(-1, 0))) {
        GameObject* obj = SceneManager::CreateGameObject(sNewObjectName);
        SceneManager::SetSelectedObject(obj);
    }

    ImGui::Separator();

    // Handle Delete key
    GameObject* selected = SceneManager::GetSelectedObject();
    if (selected && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        SceneManager::DestroyGameObject(selected);
        selected = nullptr;
    }

    // Draw root objects
    for (GameObject* obj : SceneManager::GetRootObjects()) {
        DrawGameObjectNode(obj);
    }

    // Empty space click to deselect
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered()) {
        SceneManager::SetSelectedObject(nullptr);
    }

    // Empty space drop target (make root)
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
    // Transform (always present)
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        obj->GetTransform()->OnInspectorGUI();
    }

    // Other components
    for (Component* comp : obj->GetComponents()) {
        std::string header = std::string(comp->GetTypeName());

        if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            comp->OnInspectorGUI();

            ImGui::Spacing();
            if (ImGui::Button(("Remove##" + header).c_str())) {
                obj->RemoveComponent(comp);
                break; // Exit loop as we modified the list
            }
        }
    }
}

void EditorUI::DrawInspector() {
    if (!sShowInspector) return;

    ImGui::Begin("Inspector", &sShowInspector);

    GameObject* selected = SceneManager::GetSelectedObject();

    if (selected) {
        // Header with ID
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Text("ID: %u", selected->GetID());
        ImGui::PopStyleColor();

        ImGui::Separator();

        // Name
        char nameBuf[256];
        strncpy(nameBuf, selected->GetName().c_str(), sizeof(nameBuf) - 1);
        nameBuf[sizeof(nameBuf) - 1] = '\0';

        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf))) {
            selected->SetName(nameBuf);
        }
        ImGui::PopItemWidth();

        // Active checkbox
        bool active = selected->IsActive();
        if (ImGui::Checkbox("Active", &active)) {
            selected->SetActive(active);
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Components
        DrawComponentInspector(selected);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Add component button (centered)
        float buttonWidth = 200.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonWidth) * 0.5f);

        if (ImGui::Button("Add Component", ImVec2(buttonWidth, 30))) {
            ImGui::OpenPopup("AddComponentPopup");
        }

        // Add Component Popup
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

    // Meshes section
    if (ImGui::CollapsingHeader("Meshes", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& meshes = SceneManager::GetMeshList();

        if (meshes.empty()) {
            ImGui::TextDisabled("  No meshes loaded");
        }
        else {
            ImGui::Indent();
            for (const auto& mesh : meshes) {
                ImGui::PushID(mesh.second);

                // Icon + name
                ImGui::Text("[M]"); // Mesh icon
                ImGui::SameLine();
                ImGui::Selectable(mesh.first.c_str());

                // Drag source
                if (ImGui::BeginDragDropSource()) {
                    int index = mesh.second;
                    ImGui::SetDragDropPayload("MESH", &index, sizeof(int));
                    ImGui::Text("Mesh: %s (Index: %d)", mesh.first.c_str(), index);
                    ImGui::EndDragDropSource();
                }

                // Tooltip with details
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Mesh Index: %d", mesh.second);
                    ImGui::Text("Drag onto MeshRenderer to assign");
                    ImGui::EndTooltip();
                }

                ImGui::PopID();
            }
            ImGui::Unindent();
        }
    }

    ImGui::Spacing();

    // Textures section
    if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& textures = SceneManager::GetTextureList();

        if (textures.empty()) {
            ImGui::TextDisabled("  No textures loaded");
        }
        else {
            ImGui::Indent();
            for (const auto& tex : textures) {
                ImGui::PushID(tex.second);

                // Icon + name
                ImGui::Text("[T]"); // Texture icon
                ImGui::SameLine();
                ImGui::Selectable(tex.first.c_str());

                // Drag source
                if (ImGui::BeginDragDropSource()) {
                    Texture* ptr = tex.second;
                    ImGui::SetDragDropPayload("TEXTURE", &ptr, sizeof(Texture*));
                    ImGui::Text("Texture: %s", tex.first.c_str());
                    if (tex.second && tex.second->IsValid()) {
                        ImGui::Text("ID: %u", tex.second->GetID());
                    }
                    ImGui::EndDragDropSource();
                }

                // Tooltip with details
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    if (tex.second && tex.second->IsValid()) {
                        ImGui::Text("Texture ID: %u", tex.second->GetID());
                    }
                    else {
                        ImGui::Text("Invalid texture");
                    }
                    ImGui::Text("Drag onto MeshRenderer to assign");
                    ImGui::EndTooltip();
                }

                ImGui::PopID();
            }
            ImGui::Unindent();
        }
    }

    ImGui::End();
}

void EditorUI::HandleDragDrop() {
    GameObject* selected = SceneManager::GetSelectedObject();
    if (!selected) return;

    MeshRenderer* renderer = selected->GetComponent<MeshRenderer>();
    if (!renderer) return;

    // This would be called in the inspector or viewport
    // to handle dropping meshes/textures onto the MeshRenderer
}

bool EditorUI::IsWindowHovered() {
    return ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
}