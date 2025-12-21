// Renderer.cpp - PARTE 1: Headers, Variables estáticas, Shaders
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

// Variables estáticas
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

// ============================================================
// SHADERS
// ============================================================

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
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 uMVP;
uniform mat4 uModel;

void main() { 
    gl_Position = uMVP * vec4(aPos, 1.0);
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    TexCoord = aTexCoord;
}
)";

static const char* kModelTexturedFS = R"(#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D uTexture;
uniform bool uHasTexture;
uniform vec3 uColor;
uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec3 uLightColor;

void main() { 
    vec3 baseColor;
    
    if (uHasTexture) {
        baseColor = texture(uTexture, TexCoord).rgb;
    } else {
        baseColor = uColor;
    }
    
    // Iluminación Blinn-Phong
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * uLightColor;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * uLightColor;
    
    vec3 result = (ambient + diffuse + specular) * baseColor;
    FragColor = vec4(result, 1.0);
}
)";

// ============================================================
// FUNCIONES AUXILIARES
// ============================================================

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

// ============================================================
// INIT Y SHUTDOWN
// ============================================================

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
void Renderer::UnloadLoadedModel() {
    ClearModelData();
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

// Renderer.cpp - PARTE 2: LoadModelFromPath

bool Renderer::LoadModelFromPath(const std::string& path) {
    std::cout << "\n======================================" << std::endl;
    std::cout << "LOADING MODEL" << std::endl;
    std::cout << "======================================" << std::endl;

    ClearModelData();

    Assimp::Importer importer;

    // Flags optimizados para importar TODO
    unsigned flags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |     // USA SOLO ESTE (elimina GenNormals)
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

    // ============================================================
    // CARGAR MATERIALES Y TEXTURAS
    // ============================================================
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

    // ============================================================
    // CALCULAR BOUNDING BOX
    // ============================================================
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

    // ============================================================
    // PROCESAR MESHES CON NORMALES Y UVs
    // ============================================================
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
        bool hasNormals = aiMesh->HasNormals();

        // Calcular stride: 3 (pos) + 3 (normal) + 2 (uv)
        int stride = 3; // posición siempre
        if (hasNormals) stride += 3;
        if (hasUVs) stride += 2;

        std::vector<float> vertexData;
        vertexData.reserve(aiMesh->mNumVertices * stride);

        // Empaquetar datos: Position, Normal (si existe), UV (si existe)
        for (unsigned v = 0; v < aiMesh->mNumVertices; ++v) {
            // Posición (centrada y escalada)
            float x = (aiMesh->mVertices[v].x - sModelCenterX) * sModelScale;
            float y = (aiMesh->mVertices[v].y - sModelCenterY) * sModelScale;
            float z = (aiMesh->mVertices[v].z - sModelCenterZ) * sModelScale;

            vertexData.push_back(x);
            vertexData.push_back(y);
            vertexData.push_back(z);

            // Normales
            if (hasNormals) {
                vertexData.push_back(aiMesh->mNormals[v].x);
                vertexData.push_back(aiMesh->mNormals[v].y);
                vertexData.push_back(aiMesh->mNormals[v].z);
            }

            // UVs
            if (hasUVs) {
                vertexData.push_back(aiMesh->mTextureCoords[0][v].x);
                vertexData.push_back(aiMesh->mTextureCoords[0][v].y);
            }
        }

        // Índices
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

        // Crear buffers OpenGL
        Mesh mesh;
        mesh.hasNormals = hasNormals;
        mesh.hasUVs = hasUVs;

        glGenVertexArrays(1, &mesh.VAO);
        glGenBuffers(1, &mesh.VBO);
        glGenBuffers(1, &mesh.EBO);

        glBindVertexArray(mesh.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

        // Configurar atributos de vértices
        int offset = 0;

        // Atributo 0: Posición (siempre presente)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset * sizeof(float)));
        glEnableVertexAttribArray(0);
        offset += 3;

        // Atributo 1: Normal (si existe)
        if (hasNormals) {
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset * sizeof(float)));
            glEnableVertexAttribArray(1);
            offset += 3;
        }

        // Atributo 2: UV (si existe)
        if (hasUVs) {
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset * sizeof(float)));
            glEnableVertexAttribArray(2);
        }

        glBindVertexArray(0);

        mesh.indexCount = indices.size();
        mesh.materialIndex = aiMesh->mMaterialIndex;

        sMeshes.push_back(mesh);

        std::cout << "  Indices: " << indices.size() << " (triangles: " << (indices.size() / 3) << ")" << std::endl;
        std::cout << "  Stride: " << stride << " floats per vertex" << std::endl;
        std::cout << "  [OK] Mesh created with full vertex data" << std::endl;
    }

    std::cout << "\nMODEL LOADED: " << sMeshes.size() << " meshes" << std::endl;
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

// Renderer.cpp - PARTE 3: DrawLoadedModel con iluminación

void Renderer::DrawLoadedModel(Camera* camera) {
    if (sMeshes.empty()) {
        return;
    }

    if (!camera) {
        std::cerr << "ERROR: DrawLoadedModel called without camera!" << std::endl;
        return;
    }

    // Obtener posición de cámara para iluminación
    float camX, camY, camZ;
    camera->GetPosition(camX, camY, camZ);

    // Calcular matrices
    float V[16], P[16], PV[16], M[16], MVP[16];

    float aspect = (sViewportH > 0) ? ((float)sViewportW / (float)sViewportH) : 1.0f;

    camera->GetViewMatrix(V);
    camera->GetProjectionMatrix(P, aspect);

    // Calcular PV = P * V
    MatMul(PV, P, V);

    // Matriz modelo (identidad)
    MatIdentity(M);

    // Calcular MVP = PV * M
    MatMul(MVP, PV, M);

    // Configurar OpenGL
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    if (sCullingEnabled) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else {
        glDisable(GL_CULL_FACE);
    }

    if (sWireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Dibujar cada mesh
    for (size_t i = 0; i < sMeshes.size(); ++i) {
        const Mesh& mesh = sMeshes[i];

        // Obtener material
        const Material* mat = nullptr;
        if (mesh.materialIndex >= 0 && mesh.materialIndex < (int)sMaterials.size()) {
            mat = &sMaterials[mesh.materialIndex];
        }

        // Seleccionar shader
        bool hasTexture = mat && mat->diffuseTexture && mat->diffuseTexture->IsValid();
        bool useTexturedShader = mesh.hasNormals && (hasTexture || mesh.hasUVs);

        unsigned int program = useTexturedShader ? sModelProgramTextured : sModelProgram;

        if (program == 0) {
            std::cerr << "ERROR: Invalid shader program!" << std::endl;
            continue;
        }

        glUseProgram(program);

        // Enviar MVP
        int locMVP = glGetUniformLocation(program, "uMVP");
        if (locMVP != -1) {
            glUniformMatrix4fv(locMVP, 1, GL_FALSE, MVP);
        }

        // Si usamos shader con iluminación, enviar uniforms adicionales
        if (useTexturedShader) {
            // Enviar matriz modelo
            int locModel = glGetUniformLocation(program, "uModel");
            if (locModel != -1) {
                glUniformMatrix4fv(locModel, 1, GL_FALSE, M);
            }

            // Posición de luz (cerca de la cámara)
            int locLightPos = glGetUniformLocation(program, "uLightPos");
            if (locLightPos != -1) {
                glUniform3f(locLightPos, camX + 5.0f, camY + 10.0f, camZ + 5.0f);
            }

            // Posición de la vista
            int locViewPos = glGetUniformLocation(program, "uViewPos");
            if (locViewPos != -1) {
                glUniform3f(locViewPos, camX, camY, camZ);
            }

            // Color de luz
            int locLightColor = glGetUniformLocation(program, "uLightColor");
            if (locLightColor != -1) {
                glUniform3f(locLightColor, 1.0f, 1.0f, 1.0f);
            }
        }

        // Configurar textura/color
        if (hasTexture) {
            mat->diffuseTexture->Bind(0);

            int locTex = glGetUniformLocation(program, "uTexture");
            if (locTex != -1) glUniform1i(locTex, 0);

            int locHasTex = glGetUniformLocation(program, "uHasTexture");
            if (locHasTex != -1) glUniform1i(locHasTex, 1);
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
                color[0] = 0.8f; color[1] = 0.8f; color[2] = 0.8f;  // Gris
            }
            glUniform3fv(locColor, 1, color);
        }

        // DIBUJAR
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        if (hasTexture) {
            mat->diffuseTexture->Unbind();
        }
    }

    // Restaurar estado
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
}

void Renderer::ToggleWireframe() {
    sWireframeMode = !sWireframeMode;
    std::cout << "Wireframe: " << (sWireframeMode ? "ON" : "OFF") << std::endl;
}

void Renderer::ToggleCulling() {
    sCullingEnabled = !sCullingEnabled;
    std::cout << "Culling: " << (sCullingEnabled ? "ON" : "OFF") << std::endl;
}

// Renderer.cpp - PARTE 4: Funciones de Debug

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