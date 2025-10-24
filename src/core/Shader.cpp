#include "Shader.h"
#include <glad/glad.h>
#include <iostream>
#include <vector>

Shader::~Shader() {
    if (m_Program) {
        glDeleteProgram(m_Program);
        m_Program = 0;
    }
}

bool Shader::CompileFromSource(const char* vertexSrc, const char* fragmentSrc) {
    unsigned int vs = 0, fs = 0;
    if (!CompileShader(vs, GL_VERTEX_SHADER, vertexSrc)) return false;
    if (!CompileShader(fs, GL_FRAGMENT_SHADER, fragmentSrc)) { glDeleteShader(vs); return false; }

    m_Program = glCreateProgram();
    glAttachShader(m_Program, vs);
    glAttachShader(m_Program, fs);
    glLinkProgram(m_Program);

    int success = 0;
    glGetProgramiv(m_Program, GL_LINK_STATUS, &success);
    if (!success) {
        int logLen = 0;
        glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetProgramInfoLog(m_Program, logLen, nullptr, log.data());
        std::cerr << "PROGRAM LINK ERROR:\n" << log.data() << std::endl;
        glDeleteShader(vs);
        glDeleteShader(fs);
        glDeleteProgram(m_Program);
        m_Program = 0;
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return true;
}

bool Shader::CompileShader(unsigned int& outId, unsigned int type, const char* src) {
    outId = glCreateShader(type);
    glShaderSource(outId, 1, &src, nullptr);
    glCompileShader(outId);

    int success = 0;
    glGetShaderiv(outId, GL_COMPILE_STATUS, &success);
    if (!success) {
        int logLen = 0;
        glGetShaderiv(outId, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(logLen);
        glGetShaderInfoLog(outId, logLen, nullptr, log.data());
        std::cerr << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
            << " SHADER COMPILE ERROR:\n" << log.data() << std::endl;
        glDeleteShader(outId);
        outId = 0;
        return false;
    }
    return true;
}

void Shader::Use() const {
    glUseProgram(m_Program);
}

unsigned int Shader::ReleaseProgram() {
    unsigned int tmp = m_Program;
    m_Program = 0;
    return tmp;
}
