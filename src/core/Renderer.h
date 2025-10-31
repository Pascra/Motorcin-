#pragma once
#include <string>

class Camera; // Forward declaration

class Renderer {
public:
    static bool Init();
    static void Shutdown();

    static void Clear(float r, float g, float b, float a);
    static void DrawTriangle();
    static void DrawRectangleIndexed(bool wireframe);

    static void OnFileDropped(const char* path);
    static bool LoadModelFromPath(const std::string& path);
    static void DrawLoadedModel(Camera* camera); // <- CAMBIADO: recibe cámara

    static void SetViewportSize(int w, int h);
    static int GetViewportWidth() { return sViewportW; }
    static int GetViewportHeight() { return sViewportH; }

private:
    static unsigned int sProgram;
    static unsigned int sTriVAO, sTriVBO;
    static unsigned int sRectVAO, sRectVBO, sRectEBO;

    static unsigned int sModelProgram;
    static unsigned int sModelVAO, sModelVBO, sModelEBO;
    static size_t       sModelIndexCount;

    static int sViewportW, sViewportH;
};