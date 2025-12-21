#pragma once
#include <glad/glad.h>

class Model {
public:
    Model() = default;
    ~Model();

    bool LoadFromFile(const char* path); // usa Assimp
    void Draw() const;

    bool IsValid() const { return m_vao != 0 && m_vertexCount > 0; }

private:
    GLuint  m_vao = 0;
    GLuint  m_vbo = 0;
    GLsizei m_vertexCount = 0;
};
