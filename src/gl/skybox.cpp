#include "glcore/gl/skybox.hpp"

#include <glad/glad.h>

#include <cstddef>
#include <glm/glm.hpp>
#include <utility>

#include "glcore/gl/texture_loader.hpp"

namespace glcore
{
namespace gl
{

namespace
{

// 天空盒立方体的 36 个顶点（仅位置，3 float/顶点），取自原 Ark_Like/src/skybox.cpp
const float kSkyboxVertices[] = {
    // 背面 (-Z)
    -1.0f,
    1.0f,
    -1.0f,
    -1.0f,
    -1.0f,
    -1.0f,
    1.0f,
    -1.0f,
    -1.0f,  //
    1.0f,
    -1.0f,
    -1.0f,
    1.0f,
    1.0f,
    -1.0f,
    -1.0f,
    1.0f,
    -1.0f,  //
    // 左面 (-X)
    -1.0f,
    -1.0f,
    1.0f,
    -1.0f,
    -1.0f,
    -1.0f,
    -1.0f,
    1.0f,
    -1.0f,  //
    -1.0f,
    1.0f,
    -1.0f,
    -1.0f,
    1.0f,
    1.0f,
    -1.0f,
    -1.0f,
    1.0f,  //
    // 右面 (+X)
    1.0f,
    -1.0f,
    -1.0f,
    1.0f,
    -1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,  //
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    -1.0f,
    1.0f,
    -1.0f,
    -1.0f,  //
    // 正面 (+Z)
    -1.0f,
    -1.0f,
    1.0f,
    -1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,  //
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    -1.0f,
    1.0f,
    -1.0f,
    -1.0f,
    1.0f,  //
    // 顶面 (+Y)
    -1.0f,
    1.0f,
    -1.0f,
    1.0f,
    1.0f,
    -1.0f,
    1.0f,
    1.0f,
    1.0f,  //
    1.0f,
    1.0f,
    1.0f,
    -1.0f,
    1.0f,
    1.0f,
    -1.0f,
    1.0f,
    -1.0f,  //
    // 底面 (-Y)
    -1.0f,
    -1.0f,
    -1.0f,
    -1.0f,
    -1.0f,
    1.0f,
    1.0f,
    -1.0f,
    -1.0f,  //
    1.0f,
    -1.0f,
    -1.0f,
    -1.0f,
    -1.0f,
    1.0f,
    1.0f,
    -1.0f,
    1.0f  //
};

constexpr int kSkyboxVertexCount = 36;
constexpr int kSkyboxFloatsPerVertex = 3;

const char* kDefaultVertexShader =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "\n"
    "out vec3 vTexCoord;\n"
    "\n"
    "uniform mat4 projection;\n"
    "uniform mat4 view;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vTexCoord = aPos;\n"
    "    gl_Position = projection * view * vec4(aPos, 1.0);\n"
    "}\n";

const char* kDefaultFragmentShader =
    "#version 330 core\n"
    "in vec3 vTexCoord;\n"
    "\n"
    "out vec4 FragColor;\n"
    "\n"
    "uniform samplerCube skyboxTexture;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    FragColor = texture(skyboxTexture, vTexCoord);\n"
    "}\n";

}  // namespace

// ========== 构造与析构 ==========
Skybox::Skybox() : m_texture(0), m_vao(0), m_vbo(0), m_shader()
{
    m_shader.compile(defaultShaderSource());

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kSkyboxVertices), kSkyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          kSkyboxFloatsPerVertex * static_cast<GLsizei>(sizeof(float)),
                          nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Skybox::Skybox(const std::vector<std::string>& facePaths) : Skybox()
{
    load(facePaths);
}

Skybox::~Skybox()
{
    destroy();
}

Skybox::Skybox(Skybox&& other) noexcept
    : m_texture(other.m_texture),
      m_vao(other.m_vao),
      m_vbo(other.m_vbo),
      m_shader(std::move(other.m_shader))
{
    other.m_texture = 0;
    other.m_vao = 0;
    other.m_vbo = 0;
}

Skybox& Skybox::operator=(Skybox&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        m_texture = other.m_texture;
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_shader = std::move(other.m_shader);

        other.m_texture = 0;
        other.m_vao = 0;
        other.m_vbo = 0;
    }
    return *this;
}

// ========== 资源 ==========
bool Skybox::load(const std::vector<std::string>& facePaths)
{
    const unsigned int texture = TextureLoader::instance().loadCubeMap(facePaths);
    if (texture == 0)
    {
        return false;
    }

    if (m_texture != 0)
    {
        TextureLoader::instance().release(m_texture);
    }
    m_texture = texture;
    return true;
}

void Skybox::destroy()
{
    if (m_texture != 0)
    {
        TextureLoader::instance().release(m_texture);
        m_texture = 0;
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
    m_shader.destroy();
}

bool Skybox::isValid() const
{
    return m_texture != 0 && m_vao != 0 && m_shader.isValid();
}

unsigned int Skybox::textureId() const
{
    return m_texture;
}

bool Skybox::setShaderSource(const ShaderSource& source)
{
    return m_shader.compile(source);
}

// ========== 绘制 ==========
void Skybox::draw(const Camera3D& camera)
{
    if (!isValid())
    {
        return;
    }

    // 把 view 矩阵截断成旋转部分（去掉平移），使天空盒始终包裹相机。
    // glm 的 mat3(mat4) 构造只取左上 3x3，正好就是旋转部分。
    const glm::mat4 rotationView = glm::mat4(glm::mat3(camera.viewMatrix()));

    m_shader.use();
    m_shader.setInt("skyboxTexture", 1);
    m_shader.setMat4("projection", camera.projectionMatrix());
    m_shader.setMat4("view", rotationView);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, kSkyboxVertexCount);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

// ========== 默认着色器 ==========
ShaderSource Skybox::defaultShaderSource()
{
    return fromStrings(kDefaultVertexShader, kDefaultFragmentShader);
}

}  // namespace gl
}  // namespace glcore
