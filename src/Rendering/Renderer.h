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
    bool hasNormals = false;
    bool hasUVs = false;
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
    static int GetViewportWidth();
    static int GetViewportHeight();

    static bool HasLoadedModel();
    static void GetModelCenter(float& x, float& y, float& z);
    static float GetModelSize();
    static float GetModelScale();

    static void ToggleWireframe();
    static bool IsWireframeEnabled();

    static void ToggleCulling();
    static bool IsCullingEnabled();

    static void DrawDebugCube(Camera* camera, float x, float y, float z, float size);
    static void DrawDebugTriangle3D(Camera* camera);
    static void SetDebugMode(bool enabled);
    static bool IsDebugMode();
    static void UnloadLoadedModel();

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
    static float sModelScale;

    static bool sWireframeMode;
    static bool sDebugMode;
    static bool sCullingEnabled;

    static unsigned int sDebugCubeVAO;
    static unsigned int sDebugCubeVBO;
    static unsigned int sDebugTriVAO;
    static unsigned int sDebugTriVBO;

    static void ClearModelData();
    static void InitDebugCube();
    static void InitDebugTriangle3D();
};

// Funciones inline
inline int Renderer::GetViewportWidth() { return sViewportW; }
inline int Renderer::GetViewportHeight() { return sViewportH; }
inline bool Renderer::HasLoadedModel() { return !sMeshes.empty(); }
inline float Renderer::GetModelScale() { return sModelScale; }
inline bool Renderer::IsWireframeEnabled() { return sWireframeMode; }
inline bool Renderer::IsCullingEnabled() { return sCullingEnabled; }
inline void Renderer::SetDebugMode(bool enabled) { sDebugMode = enabled; }
inline bool Renderer::IsDebugMode() { return sDebugMode; }