#pragma once

#include <assimp/scene.h>

#include <map>
#include <string>
#include <vector>

#include "glcore/gl/gpu_mesh.hpp"
#include "glcore/gl/shader_program.hpp"
#include "glcore/mesh_data.hpp"

namespace glcore
{
namespace gl
{

// ========== 模型加载器 ==========
// 对应原 Ark_Like/src/model.hpp 的 Model（基于 Assimp）。
// 与原始实现的差异：
//   - 每个子网格的顶点/索引先落成核心层的 MeshData，再交给 GpuMesh 上传；
//   - 纹理按路径（含内嵌纹理的虚拟键）去重，缓存于 m_textureLoaded；
//   - 加载失败不抛异常，错误信息写入 lastError()，isLoaded() 为 false；
//   - 修正了原始代码里 switch(channels) 缺少 default 导致 format 未初始化的问题。
//
// 依赖 assimp / stb_image / glad / glm（顶点与颜色转换直接用 glm::vec3）。
// 由 GLRENDER_ENABLE_GL 控制，默认不编译。
class ModelLoader
{
   public:
    ModelLoader();
    explicit ModelLoader(const std::string& path, float scale = 1.0f);
    ~ModelLoader();

    ModelLoader(const ModelLoader&) = delete;
    ModelLoader& operator=(const ModelLoader&) = delete;
    ModelLoader(ModelLoader&&) = delete;
    ModelLoader& operator=(ModelLoader&&) = delete;

    // 重新加载；失败返回 false，详情见 lastError()
    bool load(const std::string& path, float scale = 1.0f);

    bool isLoaded() const;
    const std::string& lastError() const;
    const std::string& directory() const;

    const std::vector<GpuMesh>& meshes() const;
    std::size_t meshCount() const;

    void draw(ShaderProgram& shader) const;

    // 纹理去重缓存：路径 -> GL 纹理 id
    const std::map<std::string, unsigned int>& textureCache() const;
    std::size_t textureCacheSize() const;

   private:
    void processNode(aiNode* node, const aiScene* scene);
    GpuMesh processMesh(aiMesh* mesh, const aiScene* scene);

    std::vector<TextureInfo> loadMaterialTextures(const aiScene* scene,
                                                  aiMaterial* material,
                                                  aiTextureType type,
                                                  const std::string& typeName);

    bool textureFromEmbedded(TextureInfo& info,
                             const aiTexture* embeddedTexture,
                             const std::string& cacheKey);

    bool textureFromFile(TextureInfo& info, const std::string& path);

    std::string m_lastError;
    std::string m_directory;
    float m_scale;
    std::vector<GpuMesh> m_meshes;
    std::map<std::string, unsigned int> m_textureLoaded;
};

}  // namespace gl
}  // namespace glcore
