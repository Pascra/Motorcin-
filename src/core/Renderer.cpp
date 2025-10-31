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

// Recursos estáticos
unsigned int Renderer::sProgram = 0;
unsigned int Renderer::sTriVAO = 0;
unsigned int Renderer::sTriVBO = 0;
unsigned int Renderer::sRectVAO = 0;
unsigned int Renderer::sRectVBO = 0;
unsigned int Renderer::sRectEBO = 0;
bool Renderer::sWireframeMode = false;

unsigned int Renderer::sModelProgram = 0;
unsigned int Renderer::sModelProgramTextured = 0;


float Renderer::sModelCenterX = 0.0f;
float Renderer::sModelCenterY = 0.0f;
float Renderer::sModelCenterZ = 0.0f;
float Renderer::sModelSize = 0.0f;

std::vector<Mesh> Renderer::sMeshes;
std::vector<Material> Renderer::sMaterials;

int Renderer::sViewportW = 800;
int Renderer::sViewportH = 600;

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
void main(){ 
    gl_Position = uMVP * vec4(aPos, 1.0); 
}
)";

static const char* kModelFS = R"(#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main(){ 
    FragColor = vec4(uColor, 1.0);
}
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

// Helpers de matrices
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

    // Shader para modelo sin textura
    {
        Shader sh;
        if (!sh.CompileFromSource(kModelVS, kModelFS)) {
            std::cerr << "Model shader compile/link failed\n";
            return false;
        }
        sModelProgram = sh.ReleaseProgram();
    }

    // Shader para modelo con textura
    {
        Shader sh;
        if (!sh.CompileFromSource(kModelTexturedVS, kModelTexturedFS)) {
            std::cerr << "Model textured shader compile/link failed\n";
            return false;
        }
        sModelProgramTextured = sh.ReleaseProgram();
    }

    sInitialized = true;
    std::cout << "Renderer initialized successfully\n";
    return true;
}

void Renderer::ClearModelData() {
    // Eliminar meshes
    for (auto& mesh : sMeshes) {
        if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
        if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);
        if (mesh.EBO) glDeleteBuffers(1, &mesh.EBO);
    }
    sMeshes.clear();

    // Eliminar materiales y texturas
    for (auto& mat : sMaterials) {
        if (mat.diffuseTexture) {
            delete mat.diffuseTexture;
            mat.diffuseTexture = nullptr;
        }
    }
    sMaterials.clear();
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

    ClearModelData();

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
}

void Renderer::OnFileDropped(const char* path) {
    if (!path || !*path) return;
    std::cout << "\n=== Loading model from: " << path << " ===" << std::endl;
    LoadModelFromPath(std::string(path));
}

bool Renderer::LoadModelFromPath(const std::string& path) {
    // Limpiar modelo anterior
    ClearModelData();

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
    std::cout << "Materials: " << scene->mNumMaterials << std::endl;

    // Obtener directorio del modelo para texturas relativas
    std::filesystem::path modelPath(path);
    std::string directory = modelPath.parent_path().string();

    // Cargar materiales
    for (unsigned int m = 0; m < scene->mNumMaterials; ++m) {
        const aiMaterial* aiMat = scene->mMaterials[m];
        Material mat;

        aiColor3D color(0.8f, 0.8f, 0.8f);
        aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        mat.color[0] = color.r;
        mat.color[1] = color.g;
        mat.color[2] = color.b;

        if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texPath;
            if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                std::string fullPath = directory + "/" + texPath.C_Str();

                std::cout << "  Loading texture: " << fullPath << std::endl;

                mat.diffuseTexture = new Texture();
                if (!mat.diffuseTexture->LoadFromFile(fullPath.c_str())) {
                    std::cerr << "  Failed to load texture, using color instead\n";
                    delete mat.diffuseTexture;
                    mat.diffuseTexture = nullptr;
                }
            }
        }

        sMaterials.push_back(mat);
    }

    // Calcular bounding box global
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
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
    }

    float centerX = (minX + maxX) * 0.5f;
    float centerY = (minY + maxY) * 0.5f;
    float centerZ = (minZ + maxZ) * 0.5f;

    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;
    float maxSize = std::max({ sizeX, sizeY, sizeZ });

    // GUARDAR INFO DEL MODELO
    sModelCenterX = centerX;
    sModelCenterY = centerY;
    sModelCenterZ = centerZ;
    sModelSize = maxSize;

    std::cout << "\n*** BOUNDING BOX ***" << std::endl;
    std::cout << "Center: (" << centerX << ", " << centerY << ", " << centerZ << ")" << std::endl;
    std::cout << "Size: " << maxSize << std::endl;

    // Procesar meshes
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* aiMesh = scene->mMeshes[i];

        if (!(aiMesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE)) {
            continue;
        }

        std::cout << "Processing mesh " << i << ": " << aiMesh->mNumVertices << " vertices" << std::endl;

        std::vector<float> vertexData;
        bool hasUVs = aiMesh->HasTextureCoords(0);
        int stride = hasUVs ? 5 : 3;

        vertexData.reserve(aiMesh->mNumVertices * stride);

        for (unsigned v = 0; v < aiMesh->mNumVertices; ++v) {
            // Posición (centrada)
            vertexData.push_back(aiMesh->mVertices[v].x - centerX);
            vertexData.push_back(aiMesh->mVertices[v].y - centerY);
            vertexData.push_back(aiMesh->mVertices[v].z - centerZ);

            if (hasUVs) {
                vertexData.push_back(aiMesh->mTextureCoords[0][v].x);
                vertexData.push_back(aiMesh->mTextureCoords[0][v].y);
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

        std::cout << "  Mesh created. Indices: " << mesh.indexCount
            << ", Material: " << mesh.materialIndex
            << ", Has UVs: " << (hasUVs ? "YES" : "NO") << std::endl;
    }

    std::cout << "Model loaded successfully! Total meshes: " << sMeshes.size() << std::endl;
    return true;
}

void Renderer::GetModelCenter(float& x, float& y, float& z) {
    x = sModelCenterX;
    y = sModelCenterY;
    z = sModelCenterZ;
}

float Renderer::GetModelSize() {
    return sModelSize;
}

void Renderer::DrawLoadedModel(Camera* camera) {
    if (sMeshes.empty() || !camera) {
        return;
    }

    static int drawCallCount = 0;
    bool shouldDebug = (drawCallCount < 5 || drawCallCount % 60 == 0) && drawCallCount < 300;

    if (shouldDebug) {
        std::cout << "\n=== DRAW CALL #" << drawCallCount << " ===" << std::endl;
        std::cout << "Meshes to draw: " << sMeshes.size() << std::endl;
    }

    // Calcular MVP
    float P[16], V[16], PV[16], M[16], MVP[16];

    float aspect = 1.0f;
    if (sViewportH > 0) {
        aspect = (float)sViewportW / (float)sViewportH;
    }

    camera->GetProjectionMatrix(P, aspect);
    camera->GetViewMatrix(V);
    MatIdentity(M);

    MatMul(PV, P, V);
    MatMul(MVP, PV, M);

    if (shouldDebug) {
        std::cout << "MVP matrix first values: " << MVP[0] << ", " << MVP[1] << ", " << MVP[2] << std::endl;
    }

    // Configurar modo de renderizado
    if (sWireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f);
        glDisable(GL_CULL_FACE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glLineWidth(1.0f);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    // Dibujar cada mesh
    for (size_t i = 0; i < sMeshes.size(); ++i) {
        const Mesh& mesh = sMeshes[i];

        if (shouldDebug && i == 0) {
            std::cout << "Drawing mesh 0:" << std::endl;
            std::cout << "  VAO: " << mesh.VAO << std::endl;
            std::cout << "  IndexCount: " << mesh.indexCount << std::endl;
            std::cout << "  MaterialIndex: " << mesh.materialIndex << std::endl;
        }

        const Material* mat = nullptr;
        if (mesh.materialIndex >= 0 && mesh.materialIndex < (int)sMaterials.size()) {
            mat = &sMaterials[mesh.materialIndex];
        }

        bool hasTexture = mat && mat->diffuseTexture && mat->diffuseTexture->IsValid();
        unsigned int program = hasTexture ? sModelProgramTextured : sModelProgram;

        if (shouldDebug && i == 0) {
            std::cout << "  Using program: " << program << std::endl;
            std::cout << "  Has texture: " << (hasTexture ? "YES" : "NO") << std::endl;
            std::cout << "  Wireframe mode: " << (sWireframeMode ? "ON" : "OFF") << std::endl;
        }

        glUseProgram(program);

        // Set MVP
        int locMVP = glGetUniformLocation(program, "uMVP");
        if (locMVP != -1) {
            glUniformMatrix4fv(locMVP, 1, GL_FALSE, MVP);
        }
        else if (shouldDebug && i == 0) {
            std::cerr << "  WARNING: uMVP uniform not found!" << std::endl;
        }

        // Set material
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

        // Set color
        int locColor = glGetUniformLocation(program, "uColor");
        if (locColor != -1) {
            if (sWireframeMode) {
                // Color brillante para wireframe
                float wireColor[3] = { 0.0f, 1.0f, 0.0f };
                glUniform3fv(locColor, 1, wireColor);
            }
            else if (mat) {
                glUniform3fv(locColor, 1, mat->color);
            }
            else {
                float defaultColor[3] = { 0.8f, 0.8f, 0.8f };
                glUniform3fv(locColor, 1, defaultColor);
            }
        }

        // Dibujar
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        if (hasTexture) {
            mat->diffuseTexture->Unbind();
        }

        GLenum err = glGetError();
        if (err != GL_NO_ERROR && shouldDebug && i == 0) {
            std::cerr << "  OpenGL Error: " << err << std::endl;
        }
    }

    // Restaurar estado
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);

    drawCallCount++;
}
void Renderer::ToggleWireframe() {
    sWireframeMode = !sWireframeMode;
    std::cout << "Wireframe mode: " << (sWireframeMode ? "ON" : "OFF") << std::endl;
}