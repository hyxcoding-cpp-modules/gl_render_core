#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <vector>

#include "glcore/gl/shader_program.hpp"
#include "glcore/mesh_data.hpp"

namespace glcore
{
namespace gl
{

// ========== GPU 网格 ==========
// 对应原 Ark_Like/src/mesh.hpp 的 Mesh，输入改为核心层的 MeshData。
// VAO 布局固定为 location 0 = position(3)、1 = normal(3)、2 = uv(2)，
// 与 Vertex 的内存布局（pos3 + normal3 + uv2，32 字节）一一对应。
//
// 需要 glad 提供 OpenGL 函数指针。由 GLRENDER_ENABLE_GL 控制，默认不编译。
class GpuMesh
{
   public:
    GpuMesh();
    explicit GpuMesh(const MeshData& data);
    ~GpuMesh();

    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;

    GpuMesh(GpuMesh&& other) noexcept;
    GpuMesh& operator=(GpuMesh&& other) noexcept;

    // 上传顶点与索引到 VAO/VBO/EBO。data 必须 isValid()，否则返回 false
    bool upload(const MeshData& data);

    void destroy();

    bool isValid() const;
    unsigned int vao() const;
    std::size_t indexCount() const;

    const glm::vec3& emissive() const;
    void setEmissive(const glm::vec3& color);

    const std::vector<TextureInfo>& textures() const;
    void setTextures(const std::vector<TextureInfo>& textures);

    // 绑定纹理并设置 material.* 采样器；无纹理时改用 emissive 颜色
    void draw(ShaderProgram& shader) const;

    // 只发绘制命令，不做任何 uniform / 纹理绑定
    void drawRaw() const;

   private:
    void setupMesh();

    unsigned int m_vao;
    unsigned int m_vbo;
    unsigned int m_ebo;
    std::size_t m_indexCount;
    glm::vec3 m_emissive;
    std::vector<TextureInfo> m_textures;
};

}  // namespace gl
}  // namespace glcore
