#include "Model.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <iostream>

Model::~Model() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vbo = m_vao = 0;
    m_vertexCount = 0;
}

static void AppendMeshPositions(const aiMesh* mesh, std::vector<float>& out) {
    for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned int i = 0; i < face.mNumIndices; ++i) {
            unsigned int idx = face.mIndices[i];
            const aiVector3D& v = mesh->mVertices[idx];
            out.push_back(v.x); out.push_back(v.y); out.push_back(v.z);
        }
    }
}

bool Model::LoadFromFile(const char* path) {
    Assimp::Importer importer;
    const unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenNormals |
        aiProcess_PreTransformVertices |
        aiProcess_FlipUVs;

    const aiScene* scene = importer.ReadFile(path, flags);
    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << "\n";
        return false;
    }

    std::vector<float> positions;
    positions.reserve(100000);

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        if (!mesh->HasPositions() || !mesh->HasFaces()) continue;
        AppendMeshPositions(mesh, positions);
    }

    if (positions.empty()) {
        std::cerr << "Model has no triangles.\n";
        return false;
    }

    if (!m_vao) glGenVertexArrays(1, &m_vao);
    if (!m_vbo) glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    m_vertexCount = (GLsizei)(positions.size() / 3);
    std::cout << "Model loaded: " << m_vertexCount << " vertices\n";
    return true;
}

void Model::Draw() const {
    if (!IsValid()) return;
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    glBindVertexArray(0);
}
