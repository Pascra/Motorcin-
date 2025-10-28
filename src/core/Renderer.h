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
    static void DrawLoadedModel();

    // Viewport (llámalo desde Window al redimensionar)
    static void SetViewportSize(int w, int h);

private:
    // Shaders para tri/rect
    static unsigned int sProgram;
    static unsigned int sTriVAO, sTriVBO;
    static unsigned int sRectVAO, sRectVBO, sRectEBO;

    // Shader simple para modelo (uMVP + posición)
    static unsigned int sModelProgram;

    // Buffers del modelo cargado
    static unsigned int sModelVAO, sModelVBO, sModelEBO;
    static size_t       sModelIndexCount;

    // Estado del viewport
    static int sViewportW, sViewportH;

    // Set del MVP en el shader del modelo
    static void UseModelShaderAndSetMVP();
};
