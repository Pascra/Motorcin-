#include "Texture.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture()
    : mTextureID(0)
    , mWidth(0)
    , mHeight(0)
    , mChannels(0)
{
}

Texture::~Texture() {
    if (mTextureID) {
        glDeleteTextures(1, &mTextureID);
        mTextureID = 0;
    }
}

bool Texture::LoadFromFile(const char* path) {
    if (!path || !*path) {
        std::cerr << "Invalid texture path\n";
        return false;
    }

    // Liberar textura anterior si existe
    if (mTextureID) {
        glDeleteTextures(1, &mTextureID);
        mTextureID = 0;
    }

    // Cargar imagen con stb_image
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &mWidth, &mHeight, &mChannels, 0);

    if (!data) {
        std::cerr << "Failed to load texture: " << path << "\n";
        std::cerr << "STB Error: " << stbi_failure_reason() << "\n";
        return false;
    }

    std::cout << "Texture loaded: " << path << std::endl;
    std::cout << "  Size: " << mWidth << "x" << mHeight << std::endl;
    std::cout << "  Channels: " << mChannels << std::endl;

    // Crear textura OpenGL
    glGenTextures(1, &mTextureID);
    glBindTexture(GL_TEXTURE_2D, mTextureID);

    // Configurar parámetros
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Determinar formato
    GLenum format = GL_RGB;
    if (mChannels == 1)
        format = GL_RED;
    else if (mChannels == 3)
        format = GL_RGB;
    else if (mChannels == 4)
        format = GL_RGBA;

    // Subir datos
    glTexImage2D(GL_TEXTURE_2D, 0, format, mWidth, mHeight, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    std::cout << "Texture ID: " << mTextureID << std::endl;
    return true;
}

void Texture::Bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, mTextureID);
}

void Texture::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}