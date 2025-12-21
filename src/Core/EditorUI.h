#pragma once
#include <string>

class GameObject;

class EditorUI {
public:
    static void Init();
    static void Shutdown();

    static void BeginFrame();
    static void EndFrame();

    // Windows
    static void DrawHierarchy();
    static void DrawInspector();
    static void DrawMainMenuBar();
    static void DrawResourceBrowser();

    // Drag & Drop
    static void HandleDragDrop();

    // State queries
    static bool IsWindowHovered();
    static bool ShouldExit();

private:
    static void DrawGameObjectNode(GameObject* obj);
    static void DrawComponentInspector(GameObject* obj);

    // UI state
    static bool sShowHierarchy;
    static bool sShowInspector;
    static bool sShowResources;
    static bool sShouldExit;

    static GameObject* sDraggedObject;
    static std::string sNewObjectName;
};