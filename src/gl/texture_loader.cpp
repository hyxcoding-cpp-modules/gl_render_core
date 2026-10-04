#include "glcore/gl/texture_loader.hpp"

#include <glad/glad.h>
#include <stb/stb_image.h>

#include <iostream>

namespace glcore
{
namespace gl
{

namespace
{

constexpr int kCubeFaceCount = 6;

}  // namespace

// ========== 单例 ==========
TextureLoader& TextureLoader::instance()
{
    static TextureLoader loader;
    return loader;
}

TextureLoader::TextureLoader()
{
}

TextureLoader::~TextureLoader()
{
}

// ========== 工具 ==========
unsigned int TextureLoader::formatFromChannels(int channels)
{
    switch (channels)
    {
    case 1:
        return GL_RED;
    case 2:
        return GL_RG;
    case 3:
        return GL_RGB;
    case 4:
        return GL_RGBA;
    default:
        return 0;
    }
}

// ========== 2D 纹理 ==========
unsigned int TextureLoader::load2D(const std::string& path, bool flipVertically)
{
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 0);
    stbi_set_flip_vertically_on_load(0);

    if (pixels == nullptr)
    {
        std::cerr << "[ERROR] load texture 2D failed : " << path << std::endl;
        return 0;
    }

    const unsigned int format = formatFromChannels(channels);
    if (format == 0)
    {
        std::cerr << "[ERROR] unsupported channel count " << channels << " for " << path
                  << std::endl;
        stbi_image_free(pixels);
        return 0;
    }

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 static_cast<GLint>(format),
                 width,
                 height,
                 0,
                 format,
                 GL_UNSIGNED_BYTE,
                 pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);
    return texture;
}

// ========== 立方体贴图 ==========
unsigned int TextureLoader::loadCubeMap(const std::vector<std::string>& paths, bool flipVertically)
{
    if (paths.size() != static_cast<std::size_t>(kCubeFaceCount))
    {
        std::cerr << "[ERROR] loadCubeMap 需要 6 张纹理（right/left/top/bottom/front/back）"
                  << std::endl;
        return 0;
    }

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    bool allFacesLoaded = true;
    stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);
    for (std::size_t i = 0; i < paths.size(); ++i)
    {
        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* pixels = stbi_load(paths[i].c_str(), &width, &height, &channels, 0);
        if (pixels == nullptr)
        {
            std::cerr << "[ERROR] load texture cube face failed : " << paths[i] << std::endl;
            allFacesLoaded = false;
            continue;
        }

        const unsigned int format = formatFromChannels(channels);
        if (format != 0)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(i),
                         0,
                         static_cast<GLint>(format),
                         width,
                         height,
                         0,
                         format,
                         GL_UNSIGNED_BYTE,
                         pixels);
        }
        else
        {
            std::cerr << "[ERROR] unsupported channel count " << channels << " for " << paths[i]
                      << std::endl;
            allFacesLoaded = false;
        }
        stbi_image_free(pixels);
    }
    stbi_set_flip_vertically_on_load(0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    if (!allFacesLoaded)
    {
        glDeleteTextures(1, &texture);
        return 0;
    }
    return texture;
}

void TextureLoader::release(unsigned int textureId)
{
    if (textureId != 0)
    {
        glDeleteTextures(1, &textureId);
    }
}

}  // namespace gl
}  // namespace glcore
