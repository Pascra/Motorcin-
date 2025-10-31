#pragma once
#include <string>
#include <vector>

class Camera;
class Texture;

struct Mesh {
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    size_t indexCount = 0;
    int materialIndex = -1;
};

struct Material {
    Texture* diffuseTexture = nullptr;
    float color[3] = { 0.8f, 0.8f, 0.8f };
};

class Renderer {
public:
    static bool Init();
    static void Shutdown();

    static void Clear(float r, float g, float b, float a);
    static void DrawTriangle();
    static void DrawRectangleIndexed(bool wireframe);

    static void OnFileDropped(const char* path);
    static bool LoadModelFromPath(const std::string& path);
    static void DrawLoadedModel(Camera* camera);

    static void SetViewportSize(int w, int h);
    static int GetViewportWidth() { return sViewportW; }
    static int GetViewportHeight() { return sViewportH; }

    static bool HasLoadedModel() { return !sMeshes.empty(); }
    static void GetModelCenter(float& x, float& y, float& z);
    static float GetModelSize();

    // NUEVO: Control de wireframe
    static void ToggleWireframe();
    static bool IsWireframeEnabled() { return sWireframeMode; }

private:
    static unsigned int sProgram;
    static unsigned int sTriVAO, sTriVBO;
    static unsigned int sRectVAO, sRectVBO, sRectEBO;

    static unsigned int sModelProgram;
    static unsigned int sModelProgramTextured;

    static std::vector<Mesh> sMeshes;
    static std::vector<Material> sMaterials;

    static int sViewportW, sViewportH;

    static float sModelCenterX, sModelCenterY, sModelCenterZ;
    static float sModelSize;

    static bool sWireframeMode; // NUEVO

    static void ClearModelData();
};