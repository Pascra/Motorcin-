#pragma once
#include <glad/glad.h>
#include <string>

class Texture {
public:
    Texture();
    ~Texture();

    bool LoadFromFile(const char* path);
    void Bind(unsigned int slot = 0) const;
    void Unbind() const;

    bool IsValid() const { return mTextureID != 0; }
    unsigned int GetID() const { return mTextureID; }

private:
    unsigned int mTextureID;
    int mWidth;
    int mHeight;
    int mChannels;
};