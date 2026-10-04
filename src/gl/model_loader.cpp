#include "glcore/gl/model_loader.hpp"

#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <stb/stb_image.h>

#include <assimp/Importer.hpp>
#include <cstddef>
#include <glm/glm.hpp>
#include <iostream>
#include <utility>

namespace glcore
{
namespace gl
{

namespace
{

const char* kDiffuseTypeName = "texture_diffuse";
const char* kSpecularTypeName = "texture_specular";

// 由通道数推导 GL 格式，无法识别时返回 0
unsigned int formatFromChannels(int channels)
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

// 取路径所在目录（含结尾分隔符）；无目录时返回空串
std::string directoryOf(const std::string& path)
{
    const std::size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos)
    {
        return std::string();
    }
    return path.substr(0, slash + 1);
}

}  // namespace

// ========== 构造与析构 ==========
ModelLoader::ModelLoader()
    : m_lastError(), m_directory(), m_scale(1.0f), m_meshes(), m_textureLoaded()
{
}

ModelLoader::ModelLoader(const std::string& path, float scale)
    : m_lastError(), m_directory(), m_scale(scale), m_meshes(), m_textureLoaded()
{
    load(path, scale);
}

ModelLoader::~ModelLoader()
{
    // 纹理由本类负责释放；网格的 GL 资源在 GpuMesh 析构时回收
    for (std::map<std::string, unsigned int>::const_iterator it = m_textureLoaded.begin();
         it != m_textureLoaded.end();
         ++it)
    {
        if (it->second != 0)
        {
            unsigned int textureId = it->second;
            glDeleteTextures(1, &textureId);
        }
    }
    m_textureLoaded.clear();
}

// ========== 加载 ==========
bool ModelLoader::load(const std::string& path, float scale)
{
    m_meshes.clear();
    m_lastError.clear();
    m_directory = directoryOf(path);

    Assimp::Importer importer;
    unsigned int flags = aiProcess_Triangulate;
    if (scale == 1.0f)
    {
        flags |= aiProcess_JoinIdenticalVertices;
    }
    const aiScene* scene = importer.ReadFile(path, flags);
    if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0
        || scene->mRootNode == nullptr)
    {
        m_lastError = std::string("[ERROR] ASSIMP : ") + importer.GetErrorString();
        std::cerr << m_lastError << std::endl;
        return false;
    }

    // scale 不为 1 时 Assimp 的顶点已按原始数据读入，缩放交给 processMesh 处理
    m_scale = scale;
    processNode(scene->mRootNode, scene);
    return !m_meshes.empty();
}

void ModelLoader::processNode(aiNode* node, const aiScene* scene)
{
    if (node == nullptr)
    {
        return;
    }

    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        processNode(node->mChildren[i], scene);
    }
}

GpuMesh ModelLoader::processMesh(aiMesh* mesh, const aiScene* scene)
{
    MeshData data;

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        // 与原始实现一致：位置按 scale 缩放，法线与 UV 原样保留
        const glm::vec3 position(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        const glm::vec3 scaled = position * m_scale;

        Vertex vertex;
        vertex.position = scaled;

        if (mesh->mNormals != nullptr)
        {
            vertex.normal =
                glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }

        if (mesh->mTextureCoords[0] != nullptr)
        {
            vertex.texCoordU = mesh->mTextureCoords[0][i].x;
            vertex.texCoordV = mesh->mTextureCoords[0][i].y;
        }

        data.vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
        {
            data.indices.push_back(face.mIndices[j]);
        }
    }

    std::vector<TextureInfo> textures;
    if (mesh->mMaterialIndex < scene->mNumMaterials)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        const std::vector<TextureInfo> diffuse =
            loadMaterialTextures(scene, material, aiTextureType_DIFFUSE, kDiffuseTypeName);
        const std::vector<TextureInfo> specular =
            loadMaterialTextures(scene, material, aiTextureType_SPECULAR, kSpecularTypeName);

        textures.insert(textures.end(), diffuse.begin(), diffuse.end());
        textures.insert(textures.end(), specular.begin(), specular.end());

        if (textures.empty())
        {
            // 没有纹理时退化为纯色，用材质的漫反射色当自发光
            aiColor4D color;
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
            {
                data.emissive = glm::vec3(color.r, color.g, color.b);
            }
        }
    }

    GpuMesh gpuMesh(data);
    gpuMesh.setTextures(textures);
    return gpuMesh;
}

std::vector<TextureInfo> ModelLoader::loadMaterialTextures(const aiScene* scene,
                                                           aiMaterial* material,
                                                           aiTextureType type,
                                                           const std::string& typeName)
{
    std::vector<TextureInfo> textures;
    const unsigned int count = material->GetTextureCount(type);

    for (unsigned int i = 0; i < count; ++i)
    {
        aiString path;
        if (material->GetTexture(type, i, &path) != AI_SUCCESS)
        {
            continue;
        }

        const std::string rawPath = path.C_Str();
        TextureInfo info;
        info.type = typeName;

        if (!rawPath.empty() && rawPath[0] == '*')
        {
            // 内嵌纹理：路径形如 "*0"，用模型目录 + 原始字符串作为去重键
            const std::string cacheKey = m_directory + rawPath;
            if (!textureFromEmbedded(
                    info, scene->mTextures[std::stoi(rawPath.substr(1))], cacheKey))
            {
                continue;
            }
            info.embedded = true;
        }
        else
        {
            const std::string fullPath = m_directory + rawPath;
            if (!textureFromFile(info, fullPath))
            {
                std::cerr << "[ERROR] load material texture failed : " << fullPath << std::endl;
                continue;
            }
            info.embedded = false;
        }

        textures.push_back(info);
    }
    return textures;
}

// ========== 纹理上传 ==========
bool ModelLoader::textureFromFile(TextureInfo& info, const std::string& path)
{
    std::map<std::string, unsigned int>::const_iterator cached = m_textureLoaded.find(path);
    if (cached != m_textureLoaded.end())
    {
        info.id = cached->second;
        info.path = path;
        return true;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (pixels == nullptr)
    {
        return false;
    }

    const unsigned int format = formatFromChannels(channels);
    if (format == 0)
    {
        stbi_image_free(pixels);
        return false;
    }

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

    m_textureLoaded[path] = texture;
    info.id = texture;
    info.path = path;
    return true;
}

bool ModelLoader::textureFromEmbedded(TextureInfo& info,
                                      const aiTexture* embeddedTexture,
                                      const std::string& cacheKey)
{
    if (embeddedTexture == nullptr)
    {
        std::cerr << "[ERROR] empty embedded texture" << std::endl;
        return false;
    }

    std::map<std::string, unsigned int>::const_iterator cached = m_textureLoaded.find(cacheKey);
    if (cached != m_textureLoaded.end())
    {
        info.id = cached->second;
        info.path = cacheKey;
        return true;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* imageData = nullptr;

    if (embeddedTexture->mHeight == 0)
    {
        // 压缩格式（png/jpg 等）：mWidth 为字节数
        imageData =
            stbi_load_from_memory(reinterpret_cast<const unsigned char*>(embeddedTexture->pcData),
                                  static_cast<int>(embeddedTexture->mWidth),
                                  &width,
                                  &height,
                                  &channels,
                                  0);
    }
    else
    {
        // 未压缩的原始像素：分别按 4 通道与 3 通道尝试
        const int candidates[] = {4, 3};
        for (std::size_t i = 0; i < 2; ++i)
        {
            const int tryChannels = candidates[i];
            width = static_cast<int>(embeddedTexture->mWidth);
            height = static_cast<int>(embeddedTexture->mHeight);
            channels = 0;
            imageData = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(embeddedTexture->pcData),
                width * height * tryChannels,
                &width,
                &height,
                &channels,
                0);
            if (imageData != nullptr && channels > 0)
            {
                break;
            }
            if (imageData != nullptr)
            {
                stbi_image_free(imageData);
                imageData = nullptr;
            }
        }
    }

    if (imageData == nullptr)
    {
        std::cerr << "[ERROR] stbi_load_from_memory failed" << std::endl;
        return false;
    }

    const unsigned int format = formatFromChannels(channels);
    if (format == 0)
    {
        stbi_image_free(imageData);
        return false;
    }

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
                 imageData);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(imageData);

    m_textureLoaded[cacheKey] = texture;
    info.id = texture;
    info.path = cacheKey;
    return true;
}

// ========== 查询与绘制 ==========
bool ModelLoader::isLoaded() const
{
    return !m_meshes.empty();
}

const std::string& ModelLoader::lastError() const
{
    return m_lastError;
}

const std::string& ModelLoader::directory() const
{
    return m_directory;
}

const std::vector<GpuMesh>& ModelLoader::meshes() const
{
    return m_meshes;
}

std::size_t ModelLoader::meshCount() const
{
    return m_meshes.size();
}

void ModelLoader::draw(ShaderProgram& shader) const
{
    for (std::size_t i = 0; i < m_meshes.size(); ++i)
    {
        m_meshes[i].draw(shader);
    }
}

const std::map<std::string, unsigned int>& ModelLoader::textureCache() const
{
    return m_textureLoaded;
}

std::size_t ModelLoader::textureCacheSize() const
{
    return m_textureLoaded.size();
}

}  // namespace gl
}  // namespace glcore
