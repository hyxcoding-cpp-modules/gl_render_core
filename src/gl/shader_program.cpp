#include "glcore/gl/shader_program.hpp"

#include <glad/glad.h>

#include <cstddef>
#include <glm/gtc/type_ptr.hpp>
#include <utility>
#include <vector>

namespace glcore
{
namespace gl
{

namespace
{

std::string stageName(unsigned int type)
{
    switch (type)
    {
    case GL_VERTEX_SHADER:
        return "vertex";
    case GL_FRAGMENT_SHADER:
        return "fragment";
    case GL_GEOMETRY_SHADER:
        return "geometry";
    default:
        return "unknown";
    }
}

}  // namespace

// ========== 构造与析构 ==========
ShaderProgram::ShaderProgram() : m_id(0), m_lastError(), m_cache()
{
}

ShaderProgram::ShaderProgram(const ShaderSource& source) : m_id(0), m_lastError(), m_cache()
{
    compile(source);
}

ShaderProgram::~ShaderProgram()
{
    destroy();
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : m_id(other.m_id), m_lastError(std::move(other.m_lastError)), m_cache(std::move(other.m_cache))
{
    other.m_id = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        m_id = other.m_id;
        m_lastError = std::move(other.m_lastError);
        m_cache = std::move(other.m_cache);
        other.m_id = 0;
    }
    return *this;
}

// ========== 编译 / 链接 ==========
unsigned int
ShaderProgram::compileStage(unsigned int type, const std::string& source, const char* name)
{
    const unsigned int shader = glCreateShader(type);
    if (shader == 0)
    {
        m_lastError = std::string("[ERROR] glCreateShader 失败: ") + name;
        return 0;
    }

    const GLchar* code = source.c_str();
    const GLint length = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &code, &length);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE)
    {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        const std::size_t bufferSize = logLength > 1 ? static_cast<std::size_t>(logLength) : 1u;
        std::vector<GLchar> log(bufferSize, '\0');
        glGetShaderInfoLog(shader, static_cast<GLsizei>(bufferSize), nullptr, log.data());

        m_lastError =
            "[ERROR] " + stageName(type) + " shader 编译失败 : " + std::string(log.data());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool ShaderProgram::compile(const ShaderSource& source)
{
    destroy();
    m_lastError.clear();

    if (!source.isComplete())
    {
        m_lastError = "[ERROR] 着色器源码不完整：vertex 与 fragment 都不能为空";
        return false;
    }

    const unsigned int vertexShader = compileStage(GL_VERTEX_SHADER, source.vertex, "vertex");
    if (vertexShader == 0)
    {
        return false;
    }

    const unsigned int fragmentShader =
        compileStage(GL_FRAGMENT_SHADER, source.fragment, "fragment");
    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return false;
    }

    unsigned int geometryShader = 0;
    if (source.hasGeometry())
    {
        geometryShader = compileStage(GL_GEOMETRY_SHADER, source.geometry, "geometry");
        if (geometryShader == 0)
        {
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            return false;
        }
    }

    const unsigned int program = glCreateProgram();
    if (program == 0)
    {
        m_lastError = "[ERROR] glCreateProgram 失败";
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (geometryShader != 0)
        {
            glDeleteShader(geometryShader);
        }
        return false;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    if (geometryShader != 0)
    {
        glAttachShader(program, geometryShader);
    }
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success != GL_TRUE)
    {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        const std::size_t bufferSize = logLength > 1 ? static_cast<std::size_t>(logLength) : 1u;
        std::vector<GLchar> log(bufferSize, '\0');
        glGetProgramInfoLog(program, static_cast<GLsizei>(bufferSize), nullptr, log.data());

        m_lastError = std::string("[ERROR] program 链接失败 : ") + std::string(log.data());
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (geometryShader != 0)
        {
            glDeleteShader(geometryShader);
        }
        return false;
    }

    // 链接完成后着色器对象即可释放，程序对象已持有编译结果
    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (geometryShader != 0)
    {
        glDetachShader(program, geometryShader);
        glDeleteShader(geometryShader);
    }

    m_id = program;
    m_cache.clear();
    return true;
}

void ShaderProgram::destroy()
{
    if (m_id != 0)
    {
        glDeleteProgram(m_id);
        m_id = 0;
    }
    m_cache.clear();
}

// ========== 查询 ==========
bool ShaderProgram::isValid() const
{
    return m_id != 0;
}

unsigned int ShaderProgram::id() const
{
    return m_id;
}

const std::string& ShaderProgram::lastError() const
{
    return m_lastError;
}

const ShaderUniformCache& ShaderProgram::uniformCache() const
{
    return m_cache;
}

void ShaderProgram::use() const
{
    glUseProgram(m_id);
}

// ========== uniform 设置 ==========
int ShaderProgram::uniformLocation(const std::string& name)
{
    if (m_cache.has(name))
    {
        return m_cache.getLocation(name);
    }

    const int location = glGetUniformLocation(m_id, name.c_str());
    if (location == kInvalidUniformLocation)
    {
        // 与原实现一致：查询失败不写入缓存，避免把 -1 当成有效位置
        return kInvalidUniformLocation;
    }
    m_cache.setLocation(name, location);
    return location;
}

void ShaderProgram::setInt(const std::string& name, int value)
{
    glUniform1i(uniformLocation(name), value);
}

void ShaderProgram::setFloat(const std::string& name, float value)
{
    glUniform1f(uniformLocation(name), value);
}

void ShaderProgram::setVec3(const std::string& name, const glm::vec3& value)
{
    glUniform3f(uniformLocation(name), value.x, value.y, value.z);
}

void ShaderProgram::setMat4(const std::string& name, const glm::mat4& value)
{
    // glm 为列主序，可直接传给 GL_FALSE 的 glUniformMatrix4fv
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

unsigned int ShaderProgram::uniformBlockIndex(const std::string& name)
{
    // glGetUniformBlockIndex 返回 GLuint，直接按无符号处理，避免有符号转换告警
    const unsigned int index = glGetUniformBlockIndex(m_id, name.c_str());
    if (index == GL_INVALID_INDEX)
    {
        return GL_INVALID_INDEX;
    }
    return index;
}

void ShaderProgram::uniformBlockBinding(const std::string& name, int bindingPoint)
{
    const unsigned int index = uniformBlockIndex(name);
    if (index == GL_INVALID_INDEX)
    {
        return;
    }
    glUniformBlockBinding(m_id, index, static_cast<unsigned int>(bindingPoint));
}

}  // namespace gl
}  // namespace glcore
