#include "Renderer.h"
#include "Shader.h"
#include <glad/glad.h>

#include <string>
#include <vector>
#include <iostream>
#include <cmath>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// -------------------- Recursos estáticos --------------------
unsigned int Renderer::sProgram = 0;
unsigned int Renderer::sTriVAO = 0;
unsigned int Renderer::sTriVBO = 0;
unsigned int Renderer::sRectVAO = 0;
unsigned int Renderer::sRectVBO = 0;
unsigned int Renderer::sRectEBO = 0;

unsigned int Renderer::sModelProgram = 0;
unsigned int Renderer::sModelVAO = 0;
unsigned int Renderer::sModelVBO = 0;
unsigned int Renderer::sModelEBO = 0;
size_t       Renderer::sModelIndexCount = 0;

int Renderer::sViewportW = 800;
int Renderer::sViewportH = 600;

static bool sInitialized = false;

// -------------------- Shaders embebidos ---------------------
static const char* kVertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
void main(){ gl_Position = vec4(aPos, 1.0); }
)";

static const char* kFragmentSrc = R"(#version 330 core
out vec4 FragColor;
void main(){ FragColor = vec4(1.0, 0.5, 0.2, 1.0); }
)";

static const char* kModelVS = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main(){ gl_Position = uMVP * vec4(aPos, 1.0); }
)";

static const char* kModelFS = R"(#version 330 core
out vec4 FragColor;
void main(){ FragColor = vec4(0.85, 0.85, 0.90, 1.0); }
)";

// -------------------- Helpers de matrices -------------------
static void MatIdentity(float m[16]) { for (int i = 0; i < 16; ++i) m[i] = 0.f; m[0] = m[5] = m[10] = m[15] = 1.f; }
static void MatPerspective(float m[16], float fovy, float aspect, float zn, float zf) {
    const float f = 1.0f / std::tan(fovy * 0.5f);
    for (int i = 0; i < 16; ++i) m[i] = 0.f;
    m[0] = f / aspect; m[5] = f; m[10] = (zf + zn) / (zn - zf); m[11] = -1.f; m[14] = (2.f * zf * zn) / (zn - zf);
}
static void MatTranslate(float m[16], float x, float y, float z) { MatIdentity(m); m[12] = x; m[13] = y; m[14] = z; }
static void MatScale(float m[16], float s) { MatIdentity(m); m[0] = m[5] = m[10] = s; }
static void MatMul(float o[16], const float a[16], const float b[16]) {
    float r[16];
    for (int r0 = 0; r0 < 4; ++r0)
        for (int c = 0; c < 4; ++c)
            r[c + r0 * 4] = a[0 + r0 * 4] * b[c + 0 * 4]
            + a[1 + r0 * 4] * b[c + 1 * 4]
            + a[2 + r0 * 4] * b[c + 2 * 4]
            + a[3 + r0 * 4] * b[c + 3 * 4];
    for (int i = 0; i < 16; ++i) o[i] = r[i];
}

// -------------------- Init / Shutdown -----------------------
bool Renderer::Init() {
    if (sInitialized) return true;

    // Shader tri/rect
    {
        Shader sh;
        if (!sh.CompileFromSource(kVertexSrc, kFragmentSrc)) {
            std::cerr << "Simple shader compile/link failed\n";
            return false;
        }
        sProgram = sh.ReleaseProgram();
    }

    // VAO/VBO triángulo
    {
        float v[] = { -0.5f,-0.5f,0,  0.5f,-0.5f,0,  0.0f,0.5f,0 };
        glGenVertexArrays(1, &sTriVAO);
        glGenBuffers(1, &sTriVBO);
        glBindVertexArray(sTriVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sTriVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    // VAO/VBO/EBO rectángulo
    {
        float verts[] = { 0.5f,0.5f,0,  0.5f,-0.5f,0,  -0.5f,-0.5f,0,  -0.5f,0.5f,0 };
        unsigned idx[] = { 0,1,3, 1,2,3 };
        glGenVertexArrays(1, &sRectVAO);
        glGenBuffers(1, &sRectVBO);
        glGenBuffers(1, &sRectEBO);
        glBindVertexArray(sRectVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sRectVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sRectEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    // Shader para modelo (MVP)
    {
        Shader sh;
        if (!sh.CompileFromSource(kModelVS, kModelFS)) {
            std::cerr << "Model shader compile/link failed\n";
            return false;
        }
        sModelProgram = sh.ReleaseProgram();
    }

    sInitialized = true;
    return true;
}

void Renderer::Shutdown() {
    if (!sInitialized) return;

    // Tri/rect
    if (sTriVAO) glDeleteVertexArrays(1, &sTriVAO);
    if (sTriVBO) glDeleteBuffers(1, &sTriVBO);
    if (sRectVAO) glDeleteVertexArrays(1, &sRectVAO);
    if (sRectVBO) glDeleteBuffers(1, &sRectVBO);
    if (sRectEBO) glDeleteBuffers(1, &sRectEBO);
    if (sProgram)  glDeleteProgram(sProgram);

    // Modelo
    if (sModelVAO) glDeleteVertexArrays(1, &sModelVAO);
    if (sModelVBO) glDeleteBuffers(1, &sModelVBO);
    if (sModelEBO) glDeleteBuffers(1, &sModelEBO);
    if (sModelProgram) glDeleteProgram(sModelProgram);
    sModelVAO = sModelVBO = sModelEBO = 0;
    sModelProgram = 0;
    sModelIndexCount = 0;

    sInitialized = false;
}

// -------------------- Render primitivo ----------------------
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
    if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(sProgram);
    glBindVertexArray(sRectVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);
    if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// -------------------- Viewport ------------------------------
void Renderer::SetViewportSize(int w, int h) {
    sViewportW = w; sViewportH = h;
    glViewport(0, 0, w, h);
}

// -------------------- Modelo: carga y dibujado --------------
void Renderer::OnFileDropped(const char* path) {
    if (!path || !*path) return;
    LoadModelFromPath(std::string(path));  
}


bool Renderer::LoadModelFromPath(const std::string& path) {
    Assimp::Importer importer;
    unsigned flags = aiProcess_Triangulate
        | aiProcess_JoinIdenticalVertices
        | aiProcess_GenNormals
        | aiProcess_FlipUVs;

    const aiScene* scene = importer.ReadFile(path.c_str(), flags);
    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        std::cerr << "Assimp load failed: " << importer.GetErrorString() << "\n";
        return false;
    }

    // Busca el primer mesh triangular
    const aiMesh* mesh = nullptr;
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* m = scene->mMeshes[i];
        if (m->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) { mesh = m; break; }
    }
    if (!mesh) {
        std::cerr << "No triangulated mesh found in file.\n";
        return false;
    }

    // Extrae posiciones
    std::vector<float> positions; positions.reserve(mesh->mNumVertices * 3);
    for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
        positions.push_back(mesh->mVertices[v].x);
        positions.push_back(mesh->mVertices[v].y);
        positions.push_back(mesh->mVertices[v].z);
    }

    // Extrae índices
    std::vector<unsigned> indices; indices.reserve(mesh->mNumFaces * 3);
    for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
        const aiFace& face = mesh->mFaces[f];
        if (face.mNumIndices == 3) {
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }
    }

    // Crea/actualiza VAO/VBO/EBO
    if (!sModelVAO) glGenVertexArrays(1, &sModelVAO);
    if (!sModelVBO) glGenBuffers(1, &sModelVBO);
    if (!sModelEBO) glGenBuffers(1, &sModelEBO);

    glBindVertexArray(sModelVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sModelVBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sModelEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    sModelIndexCount = indices.size();
    std::cout << "Model loaded: " << positions.size() / 3 << " vertices, " << sModelIndexCount << " indices\n";
    return true;
}

void Renderer::UseModelShaderAndSetMVP() {
    glUseProgram(sModelProgram);
    float P[16], V[16], M[16], PV[16], MVP[16];
    const float aspect = (sViewportH > 0) ? (float)sViewportW / (float)sViewportH : 1.f;
    MatPerspective(P, 45.f * 3.1415926f / 180.f, aspect, 0.1f, 100.f);
    MatTranslate(V, 0.f, 0.f, -3.f);   // cámara simple alejando la vista
    MatScale(M, 1.0f);
    MatMul(PV, P, V);
    MatMul(MVP, PV, M);

    const int loc = glGetUniformLocation(sModelProgram, "uMVP");
    glUniformMatrix4fv(loc, 1, GL_FALSE, MVP);
}

void Renderer::DrawLoadedModel() {
    if (!sModelVAO || sModelIndexCount == 0) return;
    UseModelShaderAndSetMVP();
    glBindVertexArray(sModelVAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)sModelIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
