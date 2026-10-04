#pragma once

#include <string>
#include <vector>

#include "glcore/camera3d.hpp"
#include "glcore/gl/shader_program.hpp"
#include "glcore/shader_source.hpp"

namespace glcore
{
namespace gl
{

// ========== 天空盒 ==========
// 对应原 Ark_Like/src/skybox.hpp 的 SkyBox。
// 与原始实现的差异：
//   - 立方体 VAO/VBO 与着色器改为实例成员，不再用 static 全局量（原实现无法释放、多实例互相干扰）；
//   - 纹理通过 TextureLoader 单例加载，绘制前用 view 矩阵去掉平移分量；
//   - 着色器源码可由外部注入，默认使用内置的极简天空盒着色器。
//
// 依赖 glad 与 stb_image。由 GLRENDER_ENABLE_GL 控制，默认不编译。
class Skybox
{
   public:
    Skybox();
    // 面顺序固定为 right / left / top / bottom / front / back
    explicit Skybox(const std::vector<std::string>& facePaths);
    ~Skybox();

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    Skybox(Skybox&& other) noexcept;
    Skybox& operator=(Skybox&& other) noexcept;

    // 加载六面纹理，失败返回 false（此时纹理 id 为 0）
    bool load(const std::vector<std::string>& facePaths);

    void destroy();

    bool isValid() const;
    unsigned int textureId() const;

    // 替换内置着色器源码
    bool setShaderSource(const ShaderSource& source);

    // 使用相机参数绘制。内部会自动去掉 view 矩阵的平移分量，使天空盒始终包裹相机
    void draw(const Camera3D& camera);

   private:
    static ShaderSource defaultShaderSource();

    unsigned int m_texture;
    unsigned int m_vao;
    unsigned int m_vbo;
    ShaderProgram m_shader;
};

}  // namespace gl
}  // namespace glcore
