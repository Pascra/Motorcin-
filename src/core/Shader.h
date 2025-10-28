#pragma once
#include <string>

class Shader {
public:
    Shader() = default;
    ~Shader();

    bool CompileFromSource(const char* vertexSrc, const char* fragmentSrc);
    void Use() const;
    unsigned int ReleaseProgram();
    unsigned int GetProgram() const { return m_Program; }

private:
    unsigned int m_Program = 0;
    bool CompileShader(unsigned int& outId, unsigned int type, const char* src);
};
