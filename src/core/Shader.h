#pragma once
#include <string>

class Shader {
public:
    Shader() = default;
    ~Shader();

    // Compila y enlaza desde código fuente en memoria
    bool CompileFromSource(const char* vertexSrc, const char* fragmentSrc);

    // Activa el programa (opcional si usas Renderer::sProgram)
    void Use() const;

    // Entrega la propiedad del program a quien lo llame (no se destruirá en el destructor)
    unsigned int ReleaseProgram();

    // Devuelve el id del program
    unsigned int GetProgram() const { return m_Program; }

private:
    unsigned int m_Program = 0;

    bool CompileShader(unsigned int& outId, unsigned int type, const char* src);
};
