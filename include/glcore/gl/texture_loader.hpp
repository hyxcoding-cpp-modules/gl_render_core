#pragma once

#include <string>
#include <vector>

namespace glcore
{
namespace gl
{

// ========== 纹理加载器 ==========
// 对应原 Ark_Like/src/textureLoader.hpp 的 TextureLoader 单例。
// 与原始实现的差异：
//   - 单例改为函数内静态对象（线程安全的初始化），不再手工 new / 泄漏；
//   - 加载失败时返回 0 而不是「已生成但未填充数据的纹理 id」；
//   - 增加 release() 以便显式回收纹理。
//
// 依赖 stb_image 做解码、glad 提供 GL 函数指针。由 GLRENDER_ENABLE_GL 控制，默认不编译。
class TextureLoader
{
   public:
    static TextureLoader& instance();

    TextureLoader(const TextureLoader&) = delete;
    TextureLoader& operator=(const TextureLoader&) = delete;
    TextureLoader(TextureLoader&&) = delete;
    TextureLoader& operator=(TextureLoader&&) = delete;

    // 加载 2D 纹理，失败返回 0
    unsigned int load2D(const std::string& path, bool flipVertically = false);

    // 加载立方体贴图，顺序固定为 right / left / top / bottom / front / back，失败返回 0
    unsigned int loadCubeMap(const std::vector<std::string>& paths, bool flipVertically = false);

    // 删除纹理对象
    void release(unsigned int textureId);

   private:
    TextureLoader();
    ~TextureLoader();

    // 按通道数选择 GL 内部格式；无法识别时返回 0
    static unsigned int formatFromChannels(int channels);
};

}  // namespace gl
}  // namespace glcore
