#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace glcore
{

// ========== 顶点 ==========
// 对应原项目 mesh.hpp 的 struct Vertex（glm::vec3 Position / Normal + glm::vec2 TexCoords），
// 这里同样直接用 glm 的类型。
// 顶点布局与 GPU 侧保持一致：pos3 + normal3 + uv2，共 32 字节，无填充。
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    float texCoordU;
    float texCoordV;

    Vertex() : position(0.0f), normal(0.0f), texCoordU(0.0f), texCoordV(0.0f)
    {
    }

    // position + normal，UV 置零
    Vertex(const glm::vec3& positionValue, const glm::vec3& normalValue)
        : position(positionValue), normal(normalValue), texCoordU(0.0f), texCoordV(0.0f)
    {
    }

    // position + UV，法线置零
    Vertex(const glm::vec3& positionValue, float u, float v)
        : position(positionValue), normal(0.0f), texCoordU(u), texCoordV(v)
    {
    }

    Vertex(const glm::vec3& positionValue, const glm::vec3& normalValue, float u, float v)
        : position(positionValue), normal(normalValue), texCoordU(u), texCoordV(v)
    {
    }
};

// ========== 纹理元数据 ==========
// 对应原项目 mesh.hpp 的 struct Texture，并补上路径与「是否内嵌」标记。
// id 由 GL 后端填充，核心层只做元数据，不触碰 GL。
struct TextureInfo
{
    std::string path;
    std::string type;
    unsigned int id;
    bool embedded;

    TextureInfo() : path(), type(), id(0), embedded(false)
    {
    }

    TextureInfo(const std::string& pathValue,
                const std::string& typeValue,
                unsigned int idValue,
                bool embeddedValue)
        : path(pathValue), type(typeValue), id(idValue), embedded(embeddedValue)
    {
    }
};

// ========== 网格数据 ==========
// 与 GPU 无关的纯 CPU 数据，是 MeshDataBuilder 的产物、GPU 上传的输入。
struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    glm::vec3 emissive;

    MeshData() : vertices(), indices(), emissive(1.0f, 1.0f, 1.0f)
    {
    }

    std::size_t vertexCount() const
    {
        return vertices.size();
    }

    std::size_t indexCount() const
    {
        return indices.size();
    }

    bool isEmpty() const
    {
        return vertices.empty() || indices.empty();
    }

    // indices 非空、且每个索引都小于顶点数，才算有效
    bool isValid() const
    {
        if (vertices.empty() || indices.empty())
        {
            return false;
        }
        for (std::size_t i = 0; i < indices.size(); ++i)
        {
            if (indices[i] >= vertices.size())
            {
                return false;
            }
        }
        return true;
    }

    // 按三角形法线累加再归一化，覆盖原有的 normal 字段。
    // 顶点索引不足 3 的倍数时，尾部多余索引被忽略。
    void computeNormals();
};

// ========== 网格数据构造器 ==========
// 从扁平 float 数组构造 MeshData，对应原 main.cpp 里手写的 float vertices[]。
// floatsPerVertex 只支持 3（pos）、5（pos+uv）、6（pos+normal）、8（pos+normal+uv）。
class MeshDataBuilder
{
   public:
    explicit MeshDataBuilder(int floatsPerVertex);

    // count 必须等于构造时的 floatsPerVertex，否则抛 std::invalid_argument
    void addVertex(const float* data, int count);

    void addIndex(unsigned int index);

    void addTriangle(unsigned int a, unsigned int b, unsigned int c);

    MeshData build() const;

    int floatsPerVertex() const
    {
        return m_floatsPerVertex;
    }

    std::size_t vertexCount() const
    {
        return m_vertices.size();
    }

    std::size_t indexCount() const
    {
        return m_indices.size();
    }

   private:
    int m_floatsPerVertex;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
};

// 原 main.cpp 的立方体顶点数据：36 个顶点、每个 8 个 float（pos3 + normal3 + uv2），
// 索引为 0..35 顺序排列。返回结果的 isValid() 为 true。
MeshData cubeMeshData();

}  // namespace glcore
