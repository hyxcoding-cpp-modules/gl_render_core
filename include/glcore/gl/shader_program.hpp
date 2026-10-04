#pragma once

#include <glm/glm.hpp>
#include <string>

#include "glcore/shader_source.hpp"

namespace glcore
{
namespace gl
{

// ========== 着色器程序 ==========
// 对应原 Ark_Like/src/shader.hpp 的 Shader。
// 与原始实现的差异：
//   - 源码通过核心层的 ShaderSource 传入，本类不再自己读文件；
//   - uniform 位置缓存改用核心层的 ShaderUniformCache（可单独测试）；
//   - 编译/链接失败不再直接返回半成品对象，而是返回 false 并把日志写入 lastError()。
//
// 需要 glad 提供 OpenGL 函数指针。由 GLRENDER_ENABLE_GL 控制，默认不编译。
class ShaderProgram
{
   public:
    ShaderProgram();
    explicit ShaderProgram(const ShaderSource& source);
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    // 编译并链接。失败返回 false，日志见 lastError()
    bool compile(const ShaderSource& source);

    // 删除底层 GL 对象并清空缓存
    void destroy();

    bool isValid() const;
    unsigned int id() const;

    // 最近一次编译/链接的错误日志
    const std::string& lastError() const;

    void use() const;

    // 带缓存的 uniform 位置查询，失败返回 kInvalidUniformLocation(-1)
    int uniformLocation(const std::string& name);

    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setMat4(const std::string& name, const glm::mat4& value);

    // 对应原 Shader::uniformBlockBinding
    void uniformBlockBinding(const std::string& name, int bindingPoint);

    const ShaderUniformCache& uniformCache() const;

   private:
    unsigned int compileStage(unsigned int type, const std::string& source, const char* stageName);
    unsigned int uniformBlockIndex(const std::string& name);

    unsigned int m_id;
    std::string m_lastError;
    ShaderUniformCache m_cache;
};

}  // namespace gl
}  // namespace glcore
