#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace glcore
{

// ========== 着色器源码 ==========
// 只负责「拿到源码字符串」，不接触任何 OpenGL 调用，因此可以在没有 GL 的环境里测试。
// 对应原项目 Shader 构造函数里读文件的那一段逻辑。
struct ShaderSource
{
    std::string vertex;
    std::string fragment;
    std::string geometry;

    bool hasGeometry() const
    {
        return !geometry.empty();
    }

    // vertex 与 fragment 都非空即视为完整
    bool isComplete() const
    {
        return !vertex.empty() && !fragment.empty();
    }
};

// uniform 位置无效值（glGetUniformLocation 失败时的返回值）
constexpr int kInvalidUniformLocation = -1;

// ========== 着色器源码工具 ==========
// 读文件。文件不存在、无权限或读取中途失败时返回 false，不抛异常；outContent 保持为空。
bool readFile(const std::string& path, std::string& outContent);

// 从文件读取。任一必需文件读取失败时返回「不完整」的 ShaderSource（isComplete() 为 false）。
// geometryPath 为空字符串表示不使用几何着色器。
ShaderSource fromFiles(const std::string& vertexPath,
                       const std::string& fragmentPath,
                       const std::string& geometryPath = "");

// 直接从字符串构造，便于测试与内嵌着色器。
ShaderSource fromStrings(const std::string& vertex,
                         const std::string& fragment,
                         const std::string& geometry = "");

// 去掉 // 行注释与 /* */ 块注释。
// 行为约定：
//   - 双引号与单引号字符串字面量内部的内容原样保留（其中的 // 与 /* 不会被当成注释）；
//   - 支持反斜杠转义，\" 不会提前结束字符串；
//   - 行注释被删除，行尾换行保留（避免影响 #line 与报错行号）；
//   - 块注释被整体删除，内部的换行一并删除；
//   - 未闭合的块注释视为一直延伸到源码末尾；
//   - 未闭合的字符串字面量视为一直延伸到源码末尾（原样保留）。
std::string stripComments(const std::string& source);

// ========== uniform 位置缓存 ==========
// 对应原 Shader 里的 std::map<std::string, int> m_cache。
// 与 GL 无关的纯数据结构，便于单独测试。
class ShaderUniformCache
{
   public:
    // 不存在时返回 kInvalidUniformLocation(-1)
    int getLocation(const std::string& name) const;

    // 覆盖写入同名条目
    void setLocation(const std::string& name, int location);

    bool has(const std::string& name) const;

    void clear();

    std::size_t size() const;

    // 返回按字典序排序后的名字列表，保证输出可预测
    std::vector<std::string> names() const;

   private:
    std::map<std::string, int> m_entries;
};

}  // namespace glcore
