#include "glcore/gl/gpu_mesh.hpp"

#include <glad/glad.h>

#include <cstddef>
#include <string>
#include <utility>

namespace glcore
{
namespace gl
{

namespace
{

constexpr unsigned int kPositionAttribIndex = 0;
constexpr unsigned int kNormalAttribIndex = 1;
constexpr unsigned int kTexCoordAttribIndex = 2;

const char* kDiffuseType = "texture_diffuse";
const char* kSpecularType = "texture_specular";

}  // namespace

// ========== 构造与析构 ==========
GpuMesh::GpuMesh()
    : m_vao(0), m_vbo(0), m_ebo(0), m_indexCount(0), m_emissive(1.0f, 1.0f, 1.0f), m_textures()
{
}

GpuMesh::GpuMesh(const MeshData& data)
    : m_vao(0), m_vbo(0), m_ebo(0), m_indexCount(0), m_emissive(1.0f, 1.0f, 1.0f), m_textures()
{
    m_emissive = data.emissive;
    upload(data);
}

GpuMesh::~GpuMesh()
{
    destroy();
}

GpuMesh::GpuMesh(GpuMesh&& other) noexcept
    : m_vao(other.m_vao),
      m_vbo(other.m_vbo),
      m_ebo(other.m_ebo),
      m_indexCount(other.m_indexCount),
      m_emissive(other.m_emissive),
      m_textures(std::move(other.m_textures))
{
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
    other.m_indexCount = 0;
}

GpuMesh& GpuMesh::operator=(GpuMesh&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount;
        m_emissive = other.m_emissive;
        m_textures = std::move(other.m_textures);

        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

// ========== 上传 ==========
bool GpuMesh::upload(const MeshData& data)
{
    if (!data.isValid())
    {
        return false;
    }

    destroy();
    m_emissive = data.emissive;

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(data.vertices.size() * sizeof(Vertex)),
                 data.vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(data.indices.size() * sizeof(unsigned int)),
                 data.indices.data(),
                 GL_STATIC_DRAW);

    // 用 offsetof 而不是硬编码偏移，避免 Vertex 布局变化后静默错位
    glVertexAttribPointer(kPositionAttribIndex,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<const void*>(offsetof(Vertex, position)));
    glVertexAttribPointer(kNormalAttribIndex,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<const void*>(offsetof(Vertex, normal)));
    glVertexAttribPointer(kTexCoordAttribIndex,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<const void*>(offsetof(Vertex, texCoordU)));

    glEnableVertexAttribArray(kPositionAttribIndex);
    glEnableVertexAttribArray(kNormalAttribIndex);
    glEnableVertexAttribArray(kTexCoordAttribIndex);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_indexCount = data.indices.size();
    return true;
}

// ========== VAO 布局 ==========
// 为当前已绑定的 VAO 配置顶点属性指针：0 = position(3)、1 = normal(3)、2 = uv(2)。
// 只碰布局、不碰数据，因此和 upload() 里的缓冲区上传分开：换 VBO 数据时不必重建布局。
void GpuMesh::setupMesh()
{
    // 用 offsetof 而不是硬编码偏移，避免 Vertex 布局变化后静默错位
    glVertexAttribPointer(kPositionAttribIndex,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<const void*>(offsetof(Vertex, position)));
    glVertexAttribPointer(kNormalAttribIndex,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<const void*>(offsetof(Vertex, normal)));
    glVertexAttribPointer(kTexCoordAttribIndex,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          static_cast<GLsizei>(sizeof(Vertex)),
                          reinterpret_cast<const void*>(offsetof(Vertex, texCoordU)));

    glEnableVertexAttribArray(kPositionAttribIndex);
    glEnableVertexAttribArray(kNormalAttribIndex);
    glEnableVertexAttribArray(kTexCoordAttribIndex);
}

void GpuMesh::destroy()
{
    if (m_ebo != 0)
    {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if (m_vbo != 0)
    {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao != 0)
    {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    m_indexCount = 0;
}

// ========== 查询与状态 ==========
bool GpuMesh::isValid() const
{
    return m_vao != 0;
}

unsigned int GpuMesh::vao() const
{
    return m_vao;
}

std::size_t GpuMesh::indexCount() const
{
    return m_indexCount;
}

const glm::vec3& GpuMesh::emissive() const
{
    return m_emissive;
}

void GpuMesh::setEmissive(const glm::vec3& color)
{
    m_emissive = color;
}

const std::vector<TextureInfo>& GpuMesh::textures() const
{
    return m_textures;
}

void GpuMesh::setTextures(const std::vector<TextureInfo>& textures)
{
    m_textures = textures;
}

// ========== 绘制 ==========
void GpuMesh::draw(ShaderProgram& shader) const
{
    shader.use();

    unsigned int diffuseNumber = 0;
    unsigned int specularNumber = 0;
    for (std::size_t i = 0; i < m_textures.size(); ++i)
    {
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
        glBindTexture(GL_TEXTURE_2D, m_textures[i].id);

        std::string number;
        if (m_textures[i].type == kDiffuseType)
        {
            number = std::to_string(diffuseNumber);
            ++diffuseNumber;
        }
        else if (m_textures[i].type == kSpecularType)
        {
            number = std::to_string(specularNumber);
            ++specularNumber;
        }

        shader.setInt("material." + m_textures[i].type + "[" + number + "]", static_cast<int>(i));
    }

    if (m_textures.empty())
    {
        shader.setVec3("emissive", m_emissive);
    }

    glActiveTexture(GL_TEXTURE0);
    drawRaw();
}

void GpuMesh::drawRaw() const
{
    if (m_vao == 0 || m_indexCount == 0)
    {
        return;
    }
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

}  // namespace gl
}  // namespace glcore
