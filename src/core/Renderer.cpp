#include "Renderer.h"
#include "Shader.h"
#include <glad/glad.h>

#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <limits>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Recursos estáticos
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
static float sRotationY = 0.0f;
static float sModelScale = 1.0f;
static float sCameraDistance = 5.0f;

// Shaders
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
void main(){ 
    gl_Position = uMVP * vec4(aPos, 1.0); 
}
)";

static const char* kModelFS = R"(#version 330 core
out vec4 FragColor;
void main(){ 
    FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Verde brillante
}
)";

// Helpers de matrices
static void MatIdentity(float m[16]) {
    for (int i = 0; i < 16; ++i) m[i] = 0.f;
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

static void MatPerspective(float m[16], float fovy, float aspect, float zn, float zf) {
    const float f = 1.0f / std::tan(fovy * 0.5f);
    for (int i = 0; i < 16; ++i) m[i] = 0.f;
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zf + zn) / (zn - zf);
    m[11] = -1.f;
    m[14] = (2.f * zf * zn) / (zn - zf);
}

static void MatTranslate(float m[16], float x, float y, float z) {
    MatIdentity(m);
    m[12] = x; m[13] = y; m[14] = z;
}

static void MatScale(float m[16], float s) {
    MatIdentity(m);
    m[0] = m[5] = m[10] = s;
}

static void MatRotateY(float m[16], float angleRad) {
    MatIdentity(m);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);
    m[0] = c;  m[2] = s;
    m[8] = -s; m[10] = c;
}

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

bool Renderer::Init() {
    if (sInitialized) return true;

    std::cout << "=== Renderer::Init() ===" << std::endl;

    // Shader tri/rect
    {
        Shader sh;
        if (!sh.CompileFromSource(kVertexSrc, kFragmentSrc)) {
            std::cerr << "Simple shader compile/link failed\n";
            return false;
        }
        sProgram = sh.ReleaseProgram();
        std::cout << "Simple shader program ID: " << sProgram << std::endl;
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

    // Shader para modelo
    {
        Shader sh;
        if (!sh.CompileFromSource(kModelVS, kModelFS)) {
            std::cerr << "Model shader compile/link failed\n";
            return false;
        }
        sModelProgram = sh.ReleaseProgram();
        std::cout << "Model shader program ID: " << sModelProgram << std::endl;
    }

    sInitialized = true;
    std::cout << "Renderer initialized successfully\n";
    return true;
}

void Renderer::Shutdown() {
    if (!sInitialized) return;

    if (sTriVAO) glDeleteVertexArrays(1, &sTriVAO);
    if (sTriVBO) glDeleteBuffers(1, &sTriVBO);
    if (sRectVAO) glDeleteVertexArrays(1, &sRectVAO);
    if (sRectVBO) glDeleteBuffers(1, &sRectVBO);
    if (sRectEBO) glDeleteBuffers(1, &sRectEBO);
    if (sProgram) glDeleteProgram(sProgram);

    if (sModelVAO) glDeleteVertexArrays(1, &sModelVAO);
    if (sModelVBO) glDeleteBuffers(1, &sModelVBO);
    if (sModelEBO) glDeleteBuffers(1, &sModelEBO);
    if (sModelProgram) glDeleteProgram(sModelProgram);

    sModelVAO = sModelVBO = sModelEBO = 0;
    sModelProgram = 0;
    sModelIndexCount = 0;

    sInitialized = false;
}

void Renderer::Clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

void Renderer::SetViewportSize(int w, int h) {
    sViewportW = w;
    sViewportH = h;
    glViewport(0, 0, w, h);
    std::cout << "Viewport resized to: " << w << "x" << h << std::endl;
}

void Renderer::OnFileDropped(const char* path) {
    if (!path || !*path) return;
    std::cout << "\n=== Loading model from: " << path << " ===" << std::endl;
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

    std::cout << "Scene loaded. Meshes: " << scene->mNumMeshes << std::endl;

    const aiMesh* mesh = nullptr;
    for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* m = scene->mMeshes[i];
        std::cout << "  Mesh " << i << ": " << m->mNumVertices << " vertices, "
            << m->mNumFaces << " faces" << std::endl;
        if (m->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) {
            mesh = m;
            break;
        }
    }

    if (!mesh) {
        std::cerr << "No triangulated mesh found in file.\n";
        return false;
    }

    // Calcular bounding box
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
        float x = mesh->mVertices[v].x;
        float y = mesh->mVertices[v].y;
        float z = mesh->mVertices[v].z;

        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
        if (z < minZ) minZ = z;
        if (z > maxZ) maxZ = z;
    }

    // Calcular centro del bounding box
    float centerX = (minX + maxX) * 0.5f;
    float centerY = (minY + maxY) * 0.5f;
    float centerZ = (minZ + maxZ) * 0.5f;

    // Imprimir bounding box
    std::cout << "\n*** BOUNDING BOX ***" << std::endl;
    std::cout << "X: [" << minX << ", " << maxX << "] size: " << (maxX - minX) << std::endl;
    std::cout << "Y: [" << minY << ", " << maxY << "] size: " << (maxY - minY) << std::endl;
    std::cout << "Z: [" << minZ << ", " << maxZ << "] size: " << (maxZ - minZ) << std::endl;
    std::cout << "Center: (" << centerX << ", " << centerY << ", " << centerZ << ")" << std::endl;

    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;
    float maxSize = std::max({ sizeX, sizeY, sizeZ });

    // Calcular escala para que quepa en un cubo de 2x2x2
    sModelScale = 2.0f / maxSize;
    sCameraDistance = 3.5f;

    std::cout << "Auto scale: " << sModelScale << std::endl;
    std::cout << "Camera distance: " << sCameraDistance << std::endl;

    // CENTRAR EL MODELO: Restar el centro del bounding box
    std::vector<float> positions;
    positions.reserve(mesh->mNumVertices * 3);

    for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
        positions.push_back(mesh->mVertices[v].x - centerX);
        positions.push_back(mesh->mVertices[v].y - centerY);
        positions.push_back(mesh->mVertices[v].z - centerZ);
    }

    std::vector<unsigned> indices;
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
        const aiFace& face = mesh->mFaces[f];
        if (face.mNumIndices == 3) {
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }
    }

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

    std::cout << "Model CENTERED and loaded successfully!" << std::endl;
    std::cout << "  VAO: " << sModelVAO << std::endl;
    std::cout << "  VBO: " << sModelVBO << std::endl;
    std::cout << "  EBO: " << sModelEBO << std::endl;
    std::cout << "  Vertices: " << positions.size() / 3 << std::endl;
    std::cout << "  Indices: " << sModelIndexCount << std::endl;
    std::cout << "*********************\n" << std::endl;

    return true;
}

void Renderer::UseModelShaderAndSetMVP() {
    glUseProgram(sModelProgram);

    sRotationY += 0.01f;

    float P[16], V[16], R[16], S[16], M[16], temp[16], PV[16], MVP[16];

    const float aspect = (sViewportH > 0) ? (float)sViewportW / (float)sViewportH : 1.f;
    MatPerspective(P, 45.f * 3.1415926f / 180.f, aspect, 0.1f, 100.f);

    MatTranslate(V, 0.f, 0.f, -sCameraDistance);

    MatScale(S, sModelScale);
    MatRotateY(R, sRotationY);

    MatMul(M, R, S);
    MatMul(PV, P, V);
    MatMul(MVP, PV, M);

    const int loc = glGetUniformLocation(sModelProgram, "uMVP");
    if (loc == -1) {
        static bool warned = false;
        if (!warned) {
            std::cerr << "WARNING: uMVP uniform not found!" << std::endl;
            warned = true;
        }
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, MVP);
}

void Renderer::DrawLoadedModel() {
    if (!sModelVAO || sModelIndexCount == 0) {
        return;
    }

    static int frameCount = 0;
    if (frameCount == 0) {
        std::cout << "\n=== DRAWING MODEL ===" << std::endl;
        std::cout << "VAO: " << sModelVAO << std::endl;
        std::cout << "Index count: " << sModelIndexCount << std::endl;
        std::cout << "Program: " << sModelProgram << std::endl;
    }
    frameCount++;

    // Wireframe para ver algo
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_CULL_FACE);

    UseModelShaderAndSetMVP();

    glBindVertexArray(sModelVAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)sModelIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "OpenGL Error in DrawLoadedModel: " << err << std::endl;
    }
}
