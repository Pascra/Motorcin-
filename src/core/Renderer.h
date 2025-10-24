#pragma once

class Renderer {
public:
    // Inicializa shaders, VAOs/VBOs/EBOs
    static bool Init();
    static void Shutdown();

    static void Clear(float r, float g, float b, float a);

    // Dibuja un triángulo con glDrawArrays
    static void DrawTriangle();

    // Dibuja un rectángulo indexado con glDrawElements
    static void DrawRectangleIndexed(bool wireframe);

private:
    static unsigned int sProgram;       // Shader program
    static unsigned int sTriVAO, sTriVBO;
    static unsigned int sRectVAO, sRectVBO, sRectEBO;

    static bool sInitialized;
};
