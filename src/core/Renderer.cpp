// Renderer.cpp - CLEAN VERSION
#include "Renderer.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"
#include <glad/glad.h>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <limits>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Variables estaticas
unsigned int Renderer::sProgram = 0;
unsigned int Renderer::sTriVAO = 0;
unsigned int Renderer::sTriVBO = 0;
unsigned int Renderer::sRectVAO = 0;
unsigned int Renderer::sRectVBO = 0;
unsigned int Renderer::sRectEBO = 0;
unsigned int Renderer::sModelProgram = 0;
unsigned int Renderer::sModelProgramTextured = 0;

std::vector<Mesh> Renderer::sMeshes;
std::vector<Material> Renderer::sMaterials;

int Renderer::sViewportW = 800;
int Renderer::sViewportH = 600;

float Renderer::sModelCenterX = 0.0f;
float Renderer::sModelCenterY = 0.0f;
float Renderer::sModelCenterZ = 0.0f;
float Renderer::sModelSize = 0.0f;
float Renderer::sModelScale = 1.0f;

bool Renderer::sWireframeMode = false;
bool Renderer::sDebugMode = false;
bool Renderer::sCullingEnabled = true;

unsigned int Renderer::sDebugCubeVAO = 0;
unsigned int Renderer::sDebugCubeVBO = 0;
unsigned int Renderer::sDebugTriVAO = 0;
unsigned int Renderer::sDebugTriVBO = 0;

static bool sInitialized = false;

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
void main(){ gl_Position = uMVP * vec4(aPos, 1.0); }
)";

static const char* kModelFS = R"(#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main(){ FragColor = vec4(uColor, 1.0); }
)";

static const char* kModelTexturedVS = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
uniform mat4 uMVP;
void main(){ 
    gl_Position = uMVP * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char* kModelTexturedFS = R"(#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D uTexture;
uniform bool uHasTexture;
uniform vec3 uColor;
void main(){ 
    if (uHasTexture) {
        FragColor = texture(uTexture, TexCoord);
    } else {
        FragColor = vec4(uColor, 1.0);
    }
}
)";

// Helpers
static void MatMul(float o[16], const float a[16], const float b[16]) {
    float r[16];
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            r[col + row * 4] = a[0 + row * 4] * b[col + 0 * 4]
            + a[1 + row * 4] * b[col + 1 * 4]
            + a[2 + row * 4] * b[col + 2 * 4]
            + a[3 + row * 4] * b[col + 3 * 4];
    for (int i = 0; i < 16; ++i) o[i] = r[i];
}

static void MatIdentity(float m[16]) {
    for (int i = 0; i < 16; ++i) m[i] = 0.f;
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

// Convertir matriz de Assimp a array de floats
static void AssimpToFloatMatrix(const aiMatrix4x4& from, float to[16]) {
    to[0] = from.a1; to[4] = from.a2; to[8] = from.a3; to[12] = from.a4;
    to[1] = from.b1; to[5] = from.b2; to[9] = from.b3; to[13] = from.b4;
    to[2] = from.c1; to[6] = from.c2; to[10] = from.c3; to[14] = from.c4;
    to[3] = from.d1; to[7] = from.d2; to[11] = from.d3; to[15] = from.d4;
}

// Aplicar transformacion a un vertice
static void TransformVertex(float& x, float& y, float& z, const float m[16]) {
    float tx = m[0] * x + m[4] * y + m[8] * z + m[12];
    float ty = m[1] * x + m[5] * y + m[9] * z + m[13];
    float tz = m[2] * x + m[6] * y + m[10] * z + m[14];
    x = tx; y = ty; z = tz;
}

bool Renderer::Init() {
    if (sInitialized) return true;

    Shader sh;
    if (!sh.CompileFromSource(kVertexSrc, kFragmentSrc)) return false;
    sProgram = sh.ReleaseProgram();

    float v[] = { -0.5f,-0.5f,0,  0.5f,-0.5f,0,  0.0f,0.5f,0 };
    glGenVertexArrays(1, &sTriVAO);
    glGenBuffers(1, &sTriVBO);
    glBindVertexArray(sTriVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sTriVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

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

    Shader sh2;
    if (!sh2.CompileFromSource(kModelVS, kModelFS)) return false;
    sModelProgram = sh2.ReleaseProgram();

    Shader sh3;
    if (!sh3.CompileFromSource(kModelTexturedVS, kModelTexturedFS)) return false;
    sModelProgramTextured = sh3.ReleaseProgram();

    sInitialized = true;
    std::cout << "[RENDERER] Initialized successfully" << std::endl;
    return true;
}

void Renderer::ClearModelData() {
    for (auto& mesh : sMeshes) {
        if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
        if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);
        if (mesh.EBO) glDeleteBuffers(1, &mesh.EBO);
    }
    sMeshes.clear();

    for (auto& mat : sMaterials) {
        if (mat.diffuseTexture) {
            delete mat.diffuseTexture;
            mat.diffuseTexture = nullptr;
        }
    }
    sMaterials.clear();

    sModelCenterX = sModelCenterY = sModelCenterZ = 0.0f;
    sModelSize = 0.0f;
    sModelScale = 1.0f;
}

void Renderer::Shutdown() {
    if (!sInitialized) return;
    if (sTriVAO) glDeleteVertexArrays(1, &sTriVAO);
    if (sTriVBO) glDeleteBuffers(1, &sTriVBO);
    if (sRectVAO) glDeleteVertexArrays(1, &sRectVAO);
    if (sRectVBO) glDeleteBuffers(1, &sRectVBO);
    if (sRectEBO) glDeleteBuffers(1, &sRectEBO);
    if (sProgram) glDeleteProgram(sProgram);
    if (sModelProgram) glDeleteProgram(sModelProgram);
    if (sModelProgramTextured) glDeleteProgram(sModelProgramTextured);
    if (sDebugCubeVAO) glDeleteVertexArrays(1, &sDebugCubeVAO);
    if (sDebugCubeVBO) glDeleteBuffers(1, &sDebugCubeVBO);
    if (sDebugTriVAO) glDeleteVertexArrays(1, &sDebugTriVAO);
    if (sDebugTriVBO) glDeleteBuffers(1, &sDebugTriVBO);
    ClearModelData();
    sInitialized = false;
}

void Renderer::Clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
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
}

void Renderer::OnFileDropped(const char* path) {
    if (!path || !*path) return;
    std::cout << "\n=== FILE DROPPED: " << path << " ===" << std::endl;
    LoadModelFromPath(std::string(path));
}

bool Renderer::LoadModelFromPath(const std::string& path) {
    std::cout << "\n======================================" << std::endl;
    std::cout << "LOADING MODEL" << std::endl;
    std::cout << "======================================" << std::endl;

    ClearModelData();

    Assimp::Importer importer;

    // IMPORTANTE: NO usar PreTransformVertices si queremos preservar la jerarquia
    unsigned flags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace;

    std::cout << "Loading with Assimp flags: " << flags << std::endl;

    const aiScene* scene = importer.ReadFile(path.c_str(), flags);

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        std::cerr << "ERROR: " << importer.GetErrorString() << "\n";
        return false;
    }

    std::cout << "Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "Materials: " << scene->mNumMaterials << std::endl;

    std::filesystem::path modelPath(path);
    std::string directory = modelPath.parent_path().string();
    if (directory.empty()) directory = ".";

    // Cargar materiales y texturas
    for (unsigned int m = 0; m < scene->mNumMaterials; ++m) {
        const aiMaterial* aiMat = scene->mMaterials[m];
        Material mat;

        aiColor3D color(0.8f, 0.8f, 0.8f);
        aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        mat.color[0] = color.r;
        mat.color[1] = color.g;
        mat.color[2] = color.b;

        aiString texPath;
        if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                std::string texStr = texPath.C_Str();

                // Limpiar path
                if (texStr.substr(0, 2) == "./") texStr = texStr.substr(2);
                if (texStr.substr(0, 2) == ".\\") texStr = texStr.substr(2);

                std::filesystem::path texFilePath(texStr);
                std::string texFileName = texFilePath.filename().string();

                std::vector<std::string> paths = {
                    directory + "/" + texFileName,
                    directory + "/" + texStr,
                    texStr
                };

                for (const auto& tryPath : paths) {
                    if (std::filesystem::exists(tryPath)) {
                        std::cout << "Loading texture: " << tryPath << std::endl;
                        mat.diffuseTexture = new Texture();
                        if (mat.diffuseTexture->LoadFromFile(tryPath.c_str())) {
                            break;
                        }
                        delete mat.diffuseTexture;
                        mat.diffuseTexture = nullptr;
                    }
                }
            }
        }

        sMaterials.push_back(mat);
    }

    // Calcular bounding box
    float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        if (!mesh->HasPositions()) continue;

        for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
            float x = mesh->mVertices[v].x;
            float y = mesh->mVertices[v].y;
            float z = mesh->mVertices[v].z;

            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
            minZ = std::min(minZ, z);
            maxZ = std::max(maxZ, z);
        }
    }

    sModelCenterX = (minX + maxX) * 0.5f;
    sModelCenterY = (minY + maxY) * 0.5f;
    sModelCenterZ = (minZ + maxZ) * 0.5f;

    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;
    sModelSize = std::max({ sizeX, sizeY, sizeZ });

    std::cout << "Center: (" << sModelCenterX << ", " << sModelCenterY << ", " << sModelCenterZ << ")" << std::endl;
    std::cout << "Size: " << sModelSize << std::endl;

    // Auto-scale si es muy grande
    sModelScale = 1.0f;
    if (sModelSize > 50.0f) {
        sModelScale = 10.0f / sModelSize;
        std::cout << "AUTO-SCALE: " << sModelScale << " (size " << sModelSize << " -> " << (sModelSize * sModelScale) << ")" << std::endl;
    }

    // Procesar meshes
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* aiMesh = scene->mMeshes[i];

        if (!(aiMesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE)) {
            std::cout << "WARNING: Mesh " << i << " is not triangles, skipping" << std::endl;
            continue;
        }

        std::cout << "\nMesh " << i << ":" << std::endl;
        std::cout << "  Vertices: " << aiMesh->mNumVertices << std::endl;
        std::cout << "  Faces: " << aiMesh->mNumFaces << std::endl;
        std::cout << "  Has UVs: " << (aiMesh->HasTextureCoords(0) ? "YES" : "NO") << std::endl;
        std::cout << "  Has Normals: " << (aiMesh->HasNormals() ? "YES" : "NO") << std::endl;
        std::cout << "  Material index: " << aiMesh->mMaterialIndex << std::endl;

        bool hasUVs = aiMesh->HasTextureCoords(0);
        int stride = hasUVs ? 5 : 3;

        std::vector<float> vertexData;
        vertexData.reserve(aiMesh->mNumVertices * stride);

        // Verificar sample de vertices ANTES de escalar
        if (aiMesh->mNumVertices >= 3) {
            std::cout << "  Sample vertices (original):" << std::endl;
            for (int v = 0; v < 3; ++v) {
                std::cout << "    [" << v << "]: ("
                    << aiMesh->mVertices[v].x << ", "
                    << aiMesh->mVertices[v].y << ", "
                    << aiMesh->mVertices[v].z << ")" << std::endl;
            }
        }

        for (unsigned v = 0; v < aiMesh->mNumVertices; ++v) {
            float x = (aiMesh->mVertices[v].x - sModelCenterX) * sModelScale;
            float y = (aiMesh->mVertices[v].y - sModelCenterY) * sModelScale;
            float z = (aiMesh->mVertices[v].z - sModelCenterZ) * sModelScale;

            vertexData.push_back(x);
            vertexData.push_back(y);
            vertexData.push_back(z);

            if (hasUVs) {
                vertexData.push_back(aiMesh->mTextureCoords[0][v].x);
                vertexData.push_back(aiMesh->mTextureCoords[0][v].y);
            }
        }

        // Sample DESPUES de escalar
        if (vertexData.size() >= stride * 3) {
            std::cout << "  Sample vertices (scaled):" << std::endl;
            for (int v = 0; v < 3; ++v) {
                std::cout << "    [" << v << "]: ("
                    << vertexData[v * stride + 0] << ", "
                    << vertexData[v * stride + 1] << ", "
                    << vertexData[v * stride + 2] << ")" << std::endl;
            }
        }

        std::vector<unsigned> indices;
        indices.reserve(aiMesh->mNumFaces * 3);

        for (unsigned f = 0; f < aiMesh->mNumFaces; ++f) {
            const aiFace& face = aiMesh->mFaces[f];
            if (face.mNumIndices == 3) {
                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[1]);
                indices.push_back(face.mIndices[2]);
            }
        }

        Mesh mesh;
        glGenVertexArrays(1, &mesh.VAO);
        glGenBuffers(1, &mesh.VBO);
        glGenBuffers(1, &mesh.EBO);

        glBindVertexArray(mesh.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        if (hasUVs) {
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
        }

        glBindVertexArray(0);

        mesh.indexCount = indices.size();
        mesh.materialIndex = aiMesh->mMaterialIndex;

        sMeshes.push_back(mesh);

        std::cout << "  Indices: " << indices.size() << " triangles: " << (indices.size() / 3) << std::endl;
        std::cout << "  [OK] Mesh created" << std::endl;
    }

    std::cout << "MODEL LOADED: " << sMeshes.size() << " meshes" << std::endl;
    std::cout << "======================================\n" << std::endl;

    return true;
}

void Renderer::GetModelCenter(float& x, float& y, float& z) {
    x = sModelCenterX * sModelScale;
    y = sModelCenterY * sModelScale;
    z = sModelCenterZ * sModelScale;
}

float Renderer::GetModelSize() {
    return sModelSize * sModelScale;
}

void Renderer::DrawLoadedModel(Camera* camera) {
    if (sMeshes.empty()) {
        return;
    }

    if (!camera) {
        std::cerr << " DrawLoadedModel: No camera!" << std::endl;
        return;
    }

    static int frameCount = 0;
    bool debug = (frameCount < 5 || frameCount == 60 || frameCount == 120);

    std::cout << " DRAW FRAME " << frameCount << std::endl;

    float camX, camY, camZ;
    camera->GetPosition(camX, camY, camZ);
    std::cout << "Camera: (" << camX << ", " << camY << ", " << camZ << ")" << std::endl;
    std::cout << "Viewport: " << sViewportW << "x" << sViewportH << std::endl;
    std::cout << "Drawing " << sMeshes.size() << " meshes" << std::endl;

    // 1. Calcular matrices
    float V[16], P[16], PV[16], M[16], MVP[16];

    float aspect = (sViewportH > 0) ? ((float)sViewportW / (float)sViewportH) : 1.0f;

    camera->GetViewMatrix(V);
    camera->GetProjectionMatrix(P, aspect);

    if (debug) {
        std::cout << "  Aspect ratio: " << aspect << std::endl;
        std::cout << "  V[0-3]: [" << V[0] << ", " << V[1] << ", " << V[2] << ", " << V[3] << "]" << std::endl;
        std::cout << "  P[0-3]: [" << P[0] << ", " << P[1] << ", " << P[2] << ", " << P[3] << "]" << std::endl;
    }

    // Calcular PV = P * V
    MatMul(PV, P, V);

    // Matriz modelo (identidad)
    MatIdentity(M);

    // Calcular MVP = PV * M
    MatMul(MVP, PV, M);

    if (debug) {
        std::cout << "  MVP calculated" << std::endl;
        std::cout << "  MVP[0]: " << MVP[0] << ", MVP[5]: " << MVP[5] << ", MVP[10]: " << MVP[10] << std::endl;
    }

    // 2. Configurar OpenGL
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);  // ? VER TODAS LAS CARAS

    if (debug) {
        GLboolean depthTest;
        glGetBooleanv(GL_DEPTH_TEST, &depthTest);
        std::cout << "  Depth test: " << (depthTest ? "ENABLED" : "DISABLED") << std::endl;
    }

    if (sWireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f);
        if (debug) std::cout << "  Wireframe: ON" << std::endl;
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        if (debug) std::cout << "  Wireframe: OFF" << std::endl;
    }

    // 3. Dibujar cada mesh
    for (size_t i = 0; i < sMeshes.size(); ++i) {
        const Mesh& mesh = sMeshes[i];

        if (debug) {
            std::cout << "\n  === MESH " << i << " ===" << std::endl;
            std::cout << "    VAO: " << mesh.VAO << std::endl;
            std::cout << "    VBO: " << mesh.VBO << std::endl;
            std::cout << "    EBO: " << mesh.EBO << std::endl;
            std::cout << "    Indices: " << mesh.indexCount << std::endl;
            std::cout << "    Material: " << mesh.materialIndex << std::endl;
        }

        // Obtener material
        const Material* mat = nullptr;
        if (mesh.materialIndex >= 0 && mesh.materialIndex < (int)sMaterials.size()) {
            mat = &sMaterials[mesh.materialIndex];
        }

        // Seleccionar shader
        bool hasTexture = mat && mat->diffuseTexture && mat->diffuseTexture->IsValid();
        unsigned int program = hasTexture ? sModelProgramTextured : sModelProgram;

        if (debug) {
            std::cout << "    Program: " << program << " (" << (hasTexture ? "TEXTURED" : "COLOR") << ")" << std::endl;
        }

        if (program == 0) {
            std::cerr << "    ? Invalid shader program!" << std::endl;
            continue;
        }

        glUseProgram(program);

        // Verificar programa activo
        GLint currentProg = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProg);
        if (currentProg != (GLint)program) {
            std::cerr << "    ? Program not active! Expected " << program << ", got " << currentProg << std::endl;
            continue;
        }

        if (debug) {
            std::cout << "    ? Program active" << std::endl;
        }

        // Enviar MVP
        int locMVP = glGetUniformLocation(program, "uMVP");
        if (locMVP == -1) {
            std::cerr << "    ? uMVP uniform NOT FOUND!" << std::endl;
            // NO continues aquí, algunos shaders viejos pueden no tener el uniform
        }
        else {
            glUniformMatrix4fv(locMVP, 1, GL_FALSE, MVP);
            if (debug) {
                std::cout << "    ? MVP sent (location: " << locMVP << ")" << std::endl;
            }
        }

        // Configurar textura/color
        if (hasTexture) {
            mat->diffuseTexture->Bind(0);

            int locTex = glGetUniformLocation(program, "uTexture");
            if (locTex != -1) glUniform1i(locTex, 0);

            int locHasTex = glGetUniformLocation(program, "uHasTexture");
            if (locHasTex != -1) glUniform1i(locHasTex, 1);

            if (debug) {
                std::cout << "    ? Texture bound: " << mat->diffuseTexture->GetID() << std::endl;
            }
        }
        else {
            int locHasTex = glGetUniformLocation(program, "uHasTexture");
            if (locHasTex != -1) glUniform1i(locHasTex, 0);
        }

        // Enviar color
        int locColor = glGetUniformLocation(program, "uColor");
        if (locColor != -1) {
            float color[3];
            if (sWireframeMode) {
                color[0] = 0.0f; color[1] = 1.0f; color[2] = 0.0f;  // Verde
            }
            else if (mat) {
                color[0] = mat->color[0];
                color[1] = mat->color[1];
                color[2] = mat->color[2];
            }
            else {
                color[0] = 1.0f; color[1] = 0.0f; color[2] = 1.0f;  // Magenta
            }
            glUniform3fv(locColor, 1, color);

            if (debug) {
                std::cout << "    Color: (" << color[0] << ", " << color[1] << ", " << color[2] << ")" << std::endl;
            }
        }

        // DIBUJAR
        glBindVertexArray(mesh.VAO);

        // Verificar binding
        GLint boundVAO = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVAO);

        if (boundVAO != (GLint)mesh.VAO) {
            std::cerr << "    ? VAO bind failed! Expected " << mesh.VAO << ", got " << boundVAO << std::endl;
            continue;
        }

        if (debug) {
            std::cout << "    ? VAO bound: " << boundVAO << std::endl;

            // Verificar buffers
            GLint arrayBuf = 0, elemBuf = 0;
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuf);
            glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elemBuf);
            std::cout << "    VBO active: " << arrayBuf << ", EBO active: " << elemBuf << std::endl;
        }

        if (debug) {
            std::cout << "    ?? glDrawElements(" << mesh.indexCount << " indices)..." << std::endl;
        }

        glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0);

        // VERIFICAR ERRORES
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "    ??? OpenGL ERROR: " << err << " ";
            switch (err) {
            case GL_INVALID_ENUM: std::cerr << "(GL_INVALID_ENUM)" << std::endl; break;
            case GL_INVALID_VALUE: std::cerr << "(GL_INVALID_VALUE)" << std::endl; break;
            case GL_INVALID_OPERATION: std::cerr << "(GL_INVALID_OPERATION)" << std::endl; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: std::cerr << "(GL_INVALID_FRAMEBUFFER_OPERATION)" << std::endl; break;
            case GL_OUT_OF_MEMORY: std::cerr << "(GL_OUT_OF_MEMORY)" << std::endl; break;
            default: std::cerr << "(UNKNOWN)" << std::endl; break;
            }
        }
        else {
            if (debug) {
                std::cout << "    ??? Draw successful!" << std::endl;
            }
        }

        glBindVertexArray(0);

        if (hasTexture) {
            mat->diffuseTexture->Unbind();
        }
    }

    // Restaurar estado
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);

    if (debug) {
        std::cout << "=== END DRAW FRAME " << frameCount << " ===\n" << std::endl;
    }

    frameCount++;
}

void Renderer::ToggleWireframe() {
    sWireframeMode = !sWireframeMode;
    std::cout << "Wireframe: " << (sWireframeMode ? "ON" : "OFF") << std::endl;
}

void Renderer::ToggleCulling() {
    sCullingEnabled = !sCullingEnabled;
    std::cout << "Culling: " << (sCullingEnabled ? "ON" : "OFF") << std::endl;
}

void Renderer::InitDebugTriangle3D() {
    float vertices[] = {
         0.0f,  5.0f, 0.0f,
        -5.0f, -5.0f, 0.0f,
         5.0f, -5.0f, 0.0f
    };

    glGenVertexArrays(1, &sDebugTriVAO);
    glGenBuffers(1, &sDebugTriVBO);

    glBindVertexArray(sDebugTriVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sDebugTriVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    std::cout << "[DEBUG] Triangle initialized" << std::endl;
}

void Renderer::DrawDebugTriangle3D(Camera* camera) {
    if (sDebugTriVAO == 0) InitDebugTriangle3D();

    float P[16], V[16], PV[16], M[16], MVP[16];
    float aspect = (sViewportH > 0) ? (float)sViewportW / sViewportH : 1.0f;

    camera->GetProjectionMatrix(P, aspect);
    camera->GetViewMatrix(V);
    MatIdentity(M);

    MatMul(PV, P, V);
    MatMul(MVP, PV, M);

    glUseProgram(sModelProgram);

    int locMVP = glGetUniformLocation(sModelProgram, "uMVP");
    if (locMVP != -1) glUniformMatrix4fv(locMVP, 1, GL_FALSE, MVP);

    int locColor = glGetUniformLocation(sModelProgram, "uColor");
    if (locColor != -1) {
        float debugColor[3] = { 1.0f, 1.0f, 0.0f };
        glUniform3fv(locColor, 1, debugColor);
    }

    glDisable(GL_CULL_FACE);
    glBindVertexArray(sDebugTriVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void Renderer::InitDebugCube() {
    float vertices[] = {
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f
    };

    glGenVertexArrays(1, &sDebugCubeVAO);
    glGenBuffers(1, &sDebugCubeVBO);

    glBindVertexArray(sDebugCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sDebugCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Renderer::DrawDebugCube(Camera* camera, float x, float y, float z, float size) {
    if (sDebugCubeVAO == 0) InitDebugCube();

    float P[16], V[16], PV[16], M[16], MVP[16];
    float aspect = (sViewportH > 0) ? (float)sViewportW / sViewportH : 1.0f;

    camera->GetProjectionMatrix(P, aspect);
    camera->GetViewMatrix(V);

    MatIdentity(M);
    M[0] = M[5] = M[10] = size;
    M[12] = x; M[13] = y; M[14] = z;

    MatMul(PV, P, V);
    MatMul(MVP, PV, M);

    glUseProgram(sModelProgram);

    int locMVP = glGetUniformLocation(sModelProgram, "uMVP");
    if (locMVP != -1) glUniformMatrix4fv(locMVP, 1, GL_FALSE, MVP);

    int locColor = glGetUniformLocation(sModelProgram, "uColor");
    if (locColor != -1) {
        float debugColor[3] = { 1.0f, 0.0f, 0.0f };
        glUniform3fv(locColor, 1, debugColor);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(3.0f);
    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(sDebugCubeVAO);
    unsigned int indices[] = { 0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7 };
    for (int i = 0; i < 24; i += 2) {
        glDrawArrays(GL_LINES, indices[i], 2);
    }
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
}