#include "Renderer.h"
#include "Shader.h"
#include <glad/glad.h>
#include <iostream>

// Recursos estáticos
unsigned int Renderer::sProgram = 0;
unsigned int Renderer::sTriVAO = 0;
unsigned int Renderer::sTriVBO = 0;
unsigned int Renderer::sRectVAO = 0;
unsigned int Renderer::sRectVBO = 0;
unsigned int Renderer::sRectEBO = 0;
bool Renderer::sInitialized = false;

// Shaders básicos (GLSL 330 core)
static const char* kVertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
void main(){
    gl_Position = vec4(aPos, 1.0);
}
)";

static const char* kFragmentSrc = R"(#version 330 core
out vec4 FragColor;
void main(){
    FragColor = vec4(1.0, 0.5, 0.2, 1.0); // naranja
}
)";

bool Renderer::Init() {
    if (sInitialized) return true;

    // Compilar y enlazar shader program
    Shader shader;
    if (!shader.CompileFromSource(kVertexSrc, kFragmentSrc)) {
        std::cerr << "Shader compilation/link failed\n";
        return false;
    }
    sProgram = shader.ReleaseProgram(); // Tomamos propiedad del program

    // ---------- TRIÁNGULO: VAO + VBO (glDrawArrays) ----------
    float triVertices[] = {
        -0.5f, -0.5f, 0.0f,  // izquierda-abajo
         0.5f, -0.5f, 0.0f,  // derecha-abajo
         0.0f,  0.5f, 0.0f   // arriba
    };

    glGenVertexArrays(1, &sTriVAO);
    glGenBuffers(1, &sTriVBO);

    glBindVertexArray(sTriVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sTriVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVertices), triVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // ---------- RECTÁNGULO: VAO + VBO + EBO (glDrawElements) ----------
    float rectVertices[] = {
         0.5f,  0.5f, 0.0f,  // top right
         0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left
    };
    unsigned int rectIndices[] = {
        0, 1, 3,  // primer triángulo
        1, 2, 3   // segundo triángulo
    };

    glGenVertexArrays(1, &sRectVAO);
    glGenBuffers(1, &sRectVBO);
    glGenBuffers(1, &sRectEBO);

    glBindVertexArray(sRectVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sRectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sRectEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIndices), rectIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Importante: no desbindear GL_ELEMENT_ARRAY_BUFFER antes del VAO
    glBindVertexArray(0);

    sInitialized = true;
    return true;
}

void Renderer::Shutdown() {
    if (!sInitialized) return;

    glDeleteVertexArrays(1, &sTriVAO);
    glDeleteBuffers(1, &sTriVBO);

    glDeleteVertexArrays(1, &sRectVAO);
    glDeleteBuffers(1, &sRectVBO);
    glDeleteBuffers(1, &sRectEBO);

    if (sProgram) glDeleteProgram(sProgram);

    sTriVAO = sTriVBO = sRectVAO = sRectVBO = sRectEBO = 0;
    sProgram = 0;
    sInitialized = false;
}

void Renderer::Clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::DrawTriangle() {
    glUseProgram(sProgram);
    glBindVertexArray(sTriVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void Renderer::DrawRectangleIndexed(bool wireframe) {
    if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glUseProgram(sProgram);
    glBindVertexArray(sRectVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
