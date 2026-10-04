#include "glcore/mesh_data.hpp"

#include <stdexcept>

#include "glcore/math.hpp"

namespace glcore
{

namespace
{

// 支持的顶点布局
constexpr int kFloatsPositionOnly = 3;      // pos3
constexpr int kFloatsPositionUv = 5;        // pos3 + uv2
constexpr int kFloatsPositionNormal = 6;    // pos3 + normal3
constexpr int kFloatsPositionNormalUv = 8;  // pos3 + normal3 + uv2

bool isSupportedLayout(int floatsPerVertex)
{
    return floatsPerVertex == kFloatsPositionOnly || floatsPerVertex == kFloatsPositionUv
           || floatsPerVertex == kFloatsPositionNormal
           || floatsPerVertex == kFloatsPositionNormalUv;
}

}  // namespace

// ========== 法线重算 ==========
void MeshData::computeNormals()
{
    if (vertices.empty())
    {
        return;
    }

    for (std::size_t i = 0; i < vertices.size(); ++i)
    {
        vertices[i].normal = glm::vec3(0.0f);
    }

    const std::size_t triangleCount = indices.size() / 3;
    for (std::size_t t = 0; t < triangleCount; ++t)
    {
        const unsigned int i0 = indices[t * 3];
        const unsigned int i1 = indices[t * 3 + 1];
        const unsigned int i2 = indices[t * 3 + 2];

        const glm::vec3& p0 = vertices[i0].position;
        const glm::vec3& p1 = vertices[i1].position;
        const glm::vec3& p2 = vertices[i2].position;

        // 未归一化的面法线，其长度正比于三角形面积，天然起到面积加权作用
        const glm::vec3 faceNormal = glm::cross(p1 - p0, p2 - p0);

        vertices[i0].normal += faceNormal;
        vertices[i1].normal += faceNormal;
        vertices[i2].normal += faceNormal;
    }

    for (std::size_t i = 0; i < vertices.size(); ++i)
    {
        // 退化三角形会得到零法线，用零向量安全的归一化避免 NaN
        vertices[i].normal = normalizedOrZero(vertices[i].normal);
    }
}

// ========== 构造器 ==========
MeshDataBuilder::MeshDataBuilder(int floatsPerVertex)
    : m_floatsPerVertex(floatsPerVertex), m_vertices(), m_indices()
{
    if (!isSupportedLayout(floatsPerVertex))
    {
        throw std::invalid_argument(
            "MeshDataBuilder: floatsPerVertex 只支持 3(pos) / 5(pos+uv) / 6(pos+normal) / "
            "8(pos+normal+uv)");
    }
}

void MeshDataBuilder::addVertex(const float* data, int count)
{
    if (data == nullptr)
    {
        throw std::invalid_argument("MeshDataBuilder::addVertex: data 为空指针");
    }
    if (count != m_floatsPerVertex)
    {
        throw std::invalid_argument("MeshDataBuilder::addVertex: count 与 floatsPerVertex 不一致");
    }

    Vertex vertex;
    vertex.position = glm::vec3(data[0], data[1], data[2]);

    switch (m_floatsPerVertex)
    {
    case kFloatsPositionOnly:
        break;
    case kFloatsPositionUv:
        vertex.texCoordU = data[3];
        vertex.texCoordV = data[4];
        break;
    case kFloatsPositionNormal:
        vertex.normal = glm::vec3(data[3], data[4], data[5]);
        break;
    case kFloatsPositionNormalUv:
        vertex.normal = glm::vec3(data[3], data[4], data[5]);
        vertex.texCoordU = data[6];
        vertex.texCoordV = data[7];
        break;
    default:
        // 构造时已校验，正常流程不会走到这里
        throw std::invalid_argument("MeshDataBuilder::addVertex: 非法的顶点布局");
    }

    m_vertices.push_back(vertex);
}

void MeshDataBuilder::addIndex(unsigned int index)
{
    m_indices.push_back(index);
}

void MeshDataBuilder::addTriangle(unsigned int a, unsigned int b, unsigned int c)
{
    m_indices.push_back(a);
    m_indices.push_back(b);
    m_indices.push_back(c);
}

MeshData MeshDataBuilder::build() const
{
    MeshData mesh;
    mesh.vertices = m_vertices;
    mesh.indices = m_indices;
    return mesh;
}

// ========== 内置立方体 ==========
MeshData cubeMeshData()
{
    // 数据取自原 Ark_Like/src/main.cpp 的 float vertices[]：
    // 36 个顶点、每个 8 个 float（pos3 + normal3 + uv2），按面拆分以支持硬边法线。
    static const float kCubeVertices[] = {
        // Back face
        -0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,  // Bottom-left
        0.5f,
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        1.0f,
        1.0f,  // top-right
        0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        1.0f,
        0.0f,  // bottom-right
        0.5f,
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        1.0f,
        1.0f,  // top-right
        -0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,  // bottom-left
        -0.5f,
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        0.0f,
        1.0f,  // top-left
        // Front face
        -0.5f,
        -0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,  // bottom-left
        0.5f,
        -0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        0.0f,  // bottom-right
        0.5f,
        0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f,  // top-right
        0.5f,
        0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f,  // top-right
        -0.5f,
        0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,  // top-left
        -0.5f,
        -0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,  // bottom-left
        // Left face
        -0.5f,
        0.5f,
        0.5f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,  // top-right
        -0.5f,
        0.5f,
        -0.5f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,  // top-left
        -0.5f,
        -0.5f,
        -0.5f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,  // bottom-left
        -0.5f,
        -0.5f,
        -0.5f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,  // bottom-left
        -0.5f,
        -0.5f,
        0.5f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,  // bottom-right
        -0.5f,
        0.5f,
        0.5f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,  // top-right
        // Right face
        0.5f,
        0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,  // top-left
        0.5f,
        -0.5f,
        -0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,  // bottom-right
        0.5f,
        0.5f,
        -0.5f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,  // top-right
        0.5f,
        -0.5f,
        -0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,  // bottom-right
        0.5f,
        0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,  // top-left
        0.5f,
        -0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,  // bottom-left
        // Bottom face
        -0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,  // top-right
        0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        -1.0f,
        0.0f,
        1.0f,
        1.0f,  // top-left
        0.5f,
        -0.5f,
        0.5f,
        0.0f,
        -1.0f,
        0.0f,
        1.0f,
        0.0f,  // bottom-left
        0.5f,
        -0.5f,
        0.5f,
        0.0f,
        -1.0f,
        0.0f,
        1.0f,
        0.0f,  // bottom-left
        -0.5f,
        -0.5f,
        0.5f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,  // bottom-right
        -0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,  // top-right
        // Top face
        -0.5f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,  // top-left
        0.5f,
        0.5f,
        0.5f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,  // bottom-right
        0.5f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f,  // top-right
        0.5f,
        0.5f,
        0.5f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,  // bottom-right
        -0.5f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,  // top-left
        -0.5f,
        0.5f,
        0.5f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f  // bottom-left
    };

    const int floatsPerVertex = kFloatsPositionNormalUv;
    const int totalFloats = static_cast<int>(sizeof(kCubeVertices) / sizeof(kCubeVertices[0]));
    const int vertexCount = totalFloats / floatsPerVertex;

    MeshDataBuilder builder(floatsPerVertex);
    for (int i = 0; i < vertexCount; ++i)
    {
        builder.addVertex(&kCubeVertices[i * floatsPerVertex], floatsPerVertex);
    }
    for (int i = 0; i < vertexCount; ++i)
    {
        builder.addIndex(static_cast<unsigned int>(i));
    }
    return builder.build();
}

}  // namespace glcore
