#pragma once
#include <string>

class Renderer {
public:
    // Inicialización / apagado
    static bool Init();
    static void Shutdown();

    // Limpieza y primitivas
    static void Clear(float r, float g, float b, float a);
    static void DrawTriangle();
    static void DrawRectangleIndexed(bool wireframe);

    // Modelo (drag & drop y carga desde ruta)
    static void OnFileDropped(const char* path);
    static bool LoadModelFromPath(const std::string& path);
    static void DrawLoadedModel();  // ← PÚBLICO

    // Viewport
    static void SetViewportSize(int w, int h);

private:
    static unsigned int sProgram;
    static unsigned int sTriVAO, sTriVBO;
    static unsigned int sRectVAO, sRectVBO, sRectEBO;

    static unsigned int sModelProgram;
    static unsigned int sModelVAO, sModelVBO, sModelEBO;
    static size_t       sModelIndexCount;

    static int sViewportW, sViewportH;

    static void UseModelShaderAndSetMVP();
};