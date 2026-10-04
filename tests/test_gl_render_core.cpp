#include <gtest/gtest.h>

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <string>

#include "glcore/glcore.hpp"

namespace
{

// 判断 4x4 矩阵全部元素是否都是有限值
bool isFinite(const glm::mat4& value)
{
    const float* data = glm::value_ptr(value);
    for (int i = 0; i < 16; ++i)
    {
        if (std::isnan(data[i]) || std::isinf(data[i]))
        {
            return false;
        }
    }
    return true;
}

// 按 (x, y, z, 1) 做矩阵乘法，再把结果除以 w 分量（w 为 0 时返回未归一化的 xyz）。
// 核心层不再自带 Mat4，这里用 glm 原语复现原 Mat4::transformPoint 的语义，供断言使用。
glm::vec3 transformPoint(const glm::mat4& matrix, const glm::vec3& point)
{
    const glm::vec4 result = matrix * glm::vec4(point, 1.0f);
    if (glcore::approxZero(static_cast<double>(result.w)))
    {
        return glm::vec3(result.x, result.y, result.z);
    }
    return glm::vec3(result.x / result.w, result.y / result.w, result.z / result.w);
}

}  // namespace

// ========== glm 补充工具 ==========
TEST(MathTest, IsZeroAndSafeNormalize)
{
    EXPECT_TRUE(glcore::isZero(glm::vec3(0.0f)));
    EXPECT_FALSE(glcore::isZero(glm::vec3(1.0f, 0.0f, 0.0f)));
    EXPECT_FALSE(glcore::isZero(glm::vec3(0.0f, -2.0f, 0.0f)));

    const glm::vec3 unit = glcore::normalizedOrZero(glm::vec3(0.0f, 0.0f, 4.0f));
    EXPECT_NEAR(unit.x, 0.0f, 1.0e-6f);
    EXPECT_NEAR(unit.y, 0.0f, 1.0e-6f);
    EXPECT_NEAR(unit.z, 1.0f, 1.0e-6f);
    EXPECT_NEAR(glm::length(unit), 1.0f, 1.0e-6f);

    // 零向量归一化后仍为零向量，且不产生 NaN
    const glm::vec3 zero = glcore::normalizedOrZero(glm::vec3(0.0f));
    EXPECT_TRUE(glcore::isZero(zero));
    EXPECT_FALSE(std::isnan(zero.x));
    EXPECT_FALSE(std::isnan(zero.y));
    EXPECT_FALSE(std::isnan(zero.z));
}

TEST(MathTest, ToStringVec3AndMat4)
{
    EXPECT_EQ(glcore::toString(glm::vec3(1.0f, 2.0f, 3.0f)), "(1,2,3)");
    EXPECT_EQ(glcore::toString(glm::vec3(0.0f)), "(0,0,0)");

    EXPECT_EQ(glcore::toString(glm::mat4(1.0f)),
              "[ 1 , 0 , 0 , 0 ]\n"
              "[ 0 , 1 , 0 , 0 ]\n"
              "[ 0 , 0 , 1 , 0 ]\n"
              "[ 0 , 0 , 0 , 1 ]");
}

// ========== 向量（glm::vec3）==========
TEST(Vec3Test, ArithmeticAndLength)
{
    const glm::vec3 a(1.0f, 2.0f, 3.0f);
    const glm::vec3 b(4.0f, 5.0f, 6.0f);

    EXPECT_EQ(a + b, glm::vec3(5.0f, 7.0f, 9.0f));
    EXPECT_EQ(b - a, glm::vec3(3.0f, 3.0f, 3.0f));
    EXPECT_EQ(a * 2.0f, glm::vec3(2.0f, 4.0f, 6.0f));
    EXPECT_EQ(2.0f * a, glm::vec3(2.0f, 4.0f, 6.0f));
    EXPECT_EQ(b / 2.0f, glm::vec3(2.0f, 2.5f, 3.0f));
    EXPECT_EQ(-a, glm::vec3(-1.0f, -2.0f, -3.0f));
    EXPECT_NE(a, b);

    glm::vec3 accumulated = a;
    accumulated += glm::vec3(1.0f, 1.0f, 1.0f);
    EXPECT_EQ(accumulated, glm::vec3(2.0f, 3.0f, 4.0f));
    accumulated -= glm::vec3(2.0f, 2.0f, 2.0f);
    EXPECT_EQ(accumulated, glm::vec3(0.0f, 1.0f, 2.0f));
    accumulated *= 3.0f;
    EXPECT_EQ(accumulated, glm::vec3(0.0f, 3.0f, 6.0f));
    accumulated /= 3.0f;
    EXPECT_EQ(accumulated, glm::vec3(0.0f, 1.0f, 2.0f));
}

TEST(Vec3Test, DotCrossLength)
{
    const glm::vec3 x(1.0f, 0.0f, 0.0f);
    const glm::vec3 y(0.0f, 1.0f, 0.0f);

    EXPECT_FLOAT_EQ(glm::dot(x, y), 0.0f);
    EXPECT_FLOAT_EQ(glm::dot(x, x), 1.0f);
    EXPECT_EQ(glm::cross(x, y), glm::vec3(0.0f, 0.0f, 1.0f));
    EXPECT_EQ(glm::cross(y, x), glm::vec3(0.0f, 0.0f, -1.0f));

    const glm::vec3 v(3.0f, 4.0f, 0.0f);
    EXPECT_FLOAT_EQ(glm::dot(v, v), 25.0f);
    EXPECT_FLOAT_EQ(glm::length(v), 5.0f);

    const glm::vec3 unit = glm::normalize(glm::vec3(0.0f, 0.0f, 4.0f));
    EXPECT_NEAR(unit.z, 1.0f, 1.0e-6f);
}

// ========== 矩阵（glm::mat4）==========
TEST(Mat4Test, IdentityAndColumnMajorAccess)
{
    const glm::mat4 id(1.0f);
    EXPECT_FLOAT_EQ(id[0][0], 1.0f);
    EXPECT_FLOAT_EQ(id[1][1], 1.0f);
    EXPECT_FLOAT_EQ(id[2][2], 1.0f);
    EXPECT_FLOAT_EQ(id[3][3], 1.0f);
    // glm 为列主序，m[col][row]；下面等价于原来的 at(0, 1) 与 at(1, 0)
    EXPECT_FLOAT_EQ(id[1][0], 0.0f);
    EXPECT_FLOAT_EQ(id[0][1], 0.0f);

    // 列主序：平移量位于 value_ptr()[12..14]
    const glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_FLOAT_EQ(glm::value_ptr(t)[12], 1.0f);
    EXPECT_FLOAT_EQ(glm::value_ptr(t)[13], 2.0f);
    EXPECT_FLOAT_EQ(glm::value_ptr(t)[14], 3.0f);
    // at(0, 3)：第 0 行第 3 列，即 m[3][0]
    EXPECT_FLOAT_EQ(t[3][0], 1.0f);
}

TEST(Mat4Test, MultiplyWithIdentityAndAssociativity)
{
    const glm::mat4 id(1.0f);
    const glm::mat4 a = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    const glm::mat4 b = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
    const glm::mat4 c = glm::perspective(1.0f, 1.6f, 0.1f, 100.0f);

    const glm::mat4 left = a * id;
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            EXPECT_FLOAT_EQ(left[col][row], a[col][row]);
        }
    }

    const glm::mat4 lhs = (a * b) * c;
    const glm::mat4 rhs = a * (b * c);
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            EXPECT_NEAR(lhs[col][row], rhs[col][row], 1.0e-3f);
        }
    }
}

TEST(Mat4Test, TransformPointAndWDivision)
{
    // 平移
    const glm::vec3 translated = transformPoint(
        glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f)), glm::vec3(0.0f));
    EXPECT_FLOAT_EQ(translated.x, 1.0f);
    EXPECT_FLOAT_EQ(translated.y, 2.0f);
    EXPECT_FLOAT_EQ(translated.z, 3.0f);

    // 缩放
    const glm::vec3 scaled =
        transformPoint(glm::scale(glm::mat4(1.0f), glm::vec3(2.0f)), glm::vec3(1.0f));
    EXPECT_FLOAT_EQ(scaled.x, 2.0f);
    EXPECT_FLOAT_EQ(scaled.y, 2.0f);
    EXPECT_FLOAT_EQ(scaled.z, 2.0f);

    // w = 2 时结果应被 w 归一化
    glm::mat4 wScaled(1.0f);
    wScaled[3][3] = 2.0f;
    const glm::vec3 normalized = transformPoint(wScaled, glm::vec3(2.0f, 4.0f, 6.0f));
    EXPECT_FLOAT_EQ(normalized.x, 1.0f);
    EXPECT_FLOAT_EQ(normalized.y, 2.0f);
    EXPECT_FLOAT_EQ(normalized.z, 3.0f);
}

TEST(Mat4Test, LookAtMovesEyeToOrigin)
{
    const glm::vec3 eye(1.0f, 2.0f, 3.0f);
    const glm::vec3 center(4.0f, 5.0f, 6.0f);
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    const glm::mat4 view = glm::lookAt(eye, center, up);
    const glm::vec3 origin = transformPoint(view, eye);
    EXPECT_NEAR(origin.x, 0.0f, 1.0e-5f);
    EXPECT_NEAR(origin.y, 0.0f, 1.0e-5f);
    EXPECT_NEAR(origin.z, 0.0f, 1.0e-5f);

    // 注视点应落在视图空间的 -Z 轴上
    const glm::vec3 target = transformPoint(view, center);
    EXPECT_NEAR(target.x, 0.0f, 1.0e-4f);
    EXPECT_NEAR(target.y, 0.0f, 1.0e-4f);
    EXPECT_LT(target.z, 0.0f);
}

TEST(Mat4Test, PerspectiveAspectNearFar)
{
    const float fov = 1.0f;
    const glm::mat4 wide = glm::perspective(fov, 1.0f, 0.1f, 100.0f);
    const glm::mat4 narrow = glm::perspective(fov, 2.0f, 0.1f, 100.0f);

    // aspect 只影响 x 方向的缩放，且成反比
    EXPECT_NEAR(narrow[0][0], wide[0][0] / 2.0f, 1.0e-5f);
    EXPECT_FLOAT_EQ(narrow[1][1], wide[1][1]);
    EXPECT_NE(wide[0][0], narrow[0][0]);

    // near / far 影响深度映射（w 分量恒为 -1，与 near/far 无关）
    // 注意 glm 是 m[列][行]：原来的 at(row, col) 对应这里的 m[col][row]
    const glm::mat4 nearFar = glm::perspective(fov, 1.0f, 1.0f, 50.0f);
    EXPECT_NE(nearFar[2][2], wide[2][2]);
    EXPECT_NE(nearFar[3][2], wide[3][2]);

    EXPECT_TRUE(isFinite(wide));
    EXPECT_TRUE(isFinite(nearFar));

    // 近平面映射到 NDC z = -1，远平面映射到 NDC z = +1
    const glm::vec3 nearPoint = transformPoint(wide, glm::vec3(0.0f, 0.0f, -0.1f));
    EXPECT_NEAR(nearPoint.z, -1.0f, 1.0e-4f);
    const glm::vec3 farPoint = transformPoint(wide, glm::vec3(0.0f, 0.0f, -100.0f));
    EXPECT_NEAR(farPoint.z, 1.0f, 1.0e-4f);

    // 齐次 w 分量为 -z（透视除法的来源），深度平移项为负
    EXPECT_FLOAT_EQ(wide[2][3], -1.0f);
    EXPECT_LT(wide[3][2], 0.0f);
}

// ========== ShaderSource ==========
TEST(ShaderSourceTest, ReadFileSuccessAndFailure)
{
    std::string content;
    // 测试源文件自身一定存在
    EXPECT_TRUE(glcore::readFile(std::string(__FILE__), content));
    EXPECT_FALSE(content.empty());

    std::string missing;
    EXPECT_FALSE(glcore::readFile("__no_such_shader_file_12345__.glsl", missing));
    EXPECT_TRUE(missing.empty());

    EXPECT_FALSE(glcore::readFile("", missing));
}

TEST(ShaderSourceTest, FromStringsAndCompleteness)
{
    const glcore::ShaderSource full = glcore::fromStrings("void main(){}", "void main(){}");
    EXPECT_TRUE(full.isComplete());
    EXPECT_FALSE(full.hasGeometry());

    const glcore::ShaderSource withGeometry = glcore::fromStrings("v", "f", "void main(){}");
    EXPECT_TRUE(withGeometry.hasGeometry());
    EXPECT_TRUE(withGeometry.isComplete());

    const glcore::ShaderSource empty = glcore::fromStrings("", "fragment");
    EXPECT_FALSE(empty.isComplete());

    // 文件不存在时得到的是「不完整」的 ShaderSource，而不是异常
    const glcore::ShaderSource fromDisk =
        glcore::fromFiles("__no_such_vertex__.glsl", "__no_such_fragment__.glsl");
    EXPECT_FALSE(fromDisk.isComplete());

    const glcore::ShaderSource okFromDisk =
        glcore::fromFiles(std::string(__FILE__), std::string(__FILE__));
    EXPECT_TRUE(okFromDisk.isComplete());
}

TEST(ShaderSourceTest, StripComments)
{
    const std::string source = "a // 行注释\nb /* 块\n注释 */ c\n";
    const std::string stripped = glcore::stripComments(source);
    EXPECT_EQ(stripped.find("行注释"), std::string::npos);
    EXPECT_EQ(stripped.find("块"), std::string::npos);
    EXPECT_EQ(stripped.find("注释"), std::string::npos);
    EXPECT_NE(stripped.find("a"), std::string::npos);
    EXPECT_NE(stripped.find("b"), std::string::npos);
    EXPECT_NE(stripped.find("c"), std::string::npos);

    // 行注释删除后换行保留，行数不变
    std::size_t lines = 0;
    for (std::size_t i = 0; i < stripped.size(); ++i)
    {
        if (stripped[i] == '\n')
        {
            ++lines;
        }
    }
    EXPECT_EQ(lines, 2u);
}

TEST(ShaderSourceTest, StripCommentsKeepsStringLiterals)
{
    // 字符串字面量中的 // 与 /* 不能被当成注释
    const std::string source = "const char* url = \"http://example.com\";\n// real comment\n";
    const std::string stripped = glcore::stripComments(source);
    EXPECT_NE(stripped.find("\"http://example.com\""), std::string::npos);
    EXPECT_EQ(stripped.find("real comment"), std::string::npos);

    // 转义引号不应提前结束字符串
    const std::string escaped = "const char* s = \"a \\\" // b\";\n";
    const std::string strippedEscaped = glcore::stripComments(escaped);
    EXPECT_NE(strippedEscaped.find("\"a \\\" // b\""), std::string::npos);
}

TEST(ShaderSourceTest, StripCommentsUnterminatedBlock)
{
    const std::string source = "a\n/* 没有闭合";
    const std::string stripped = glcore::stripComments(source);
    EXPECT_EQ(stripped, "a\n");
}

// ========== ShaderUniformCache ==========
TEST(ShaderUniformCacheTest, SetGetOverwrite)
{
    glcore::ShaderUniformCache cache;
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_FALSE(cache.has("model"));
    EXPECT_EQ(cache.getLocation("model"), glcore::kInvalidUniformLocation);

    cache.setLocation("model", 3);
    EXPECT_TRUE(cache.has("model"));
    EXPECT_EQ(cache.getLocation("model"), 3);
    EXPECT_EQ(cache.size(), 1u);

    // 覆盖写入
    cache.setLocation("model", 7);
    EXPECT_EQ(cache.getLocation("model"), 7);
    EXPECT_EQ(cache.size(), 1u);

    cache.clear();
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_FALSE(cache.has("model"));
    EXPECT_EQ(cache.getLocation("model"), glcore::kInvalidUniformLocation);
}

TEST(ShaderUniformCacheTest, NamesSorted)
{
    glcore::ShaderUniformCache cache;
    cache.setLocation("view", 1);
    cache.setLocation("model", 2);
    cache.setLocation("projection", 3);
    cache.setLocation("light.color", 4);

    const std::vector<std::string> names = cache.names();
    ASSERT_EQ(names.size(), 4u);
    EXPECT_EQ(names[0], "light.color");
    EXPECT_EQ(names[1], "model");
    EXPECT_EQ(names[2], "projection");
    EXPECT_EQ(names[3], "view");
}

// ========== Vertex / MeshData ==========
TEST(MeshDataTest, VertexConstructors)
{
    glcore::Vertex def;
    EXPECT_TRUE(glcore::isZero(def.position));
    EXPECT_TRUE(glcore::isZero(def.normal));
    EXPECT_FLOAT_EQ(def.texCoordU, 0.0f);
    EXPECT_FLOAT_EQ(def.texCoordV, 0.0f);

    glcore::Vertex pn(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_FLOAT_EQ(pn.normal.y, 1.0f);
    EXPECT_FLOAT_EQ(pn.texCoordU, 0.0f);

    glcore::Vertex puv(glm::vec3(1.0f, 0.0f, 0.0f), 0.25f, 0.75f);
    EXPECT_FLOAT_EQ(puv.texCoordU, 0.25f);
    EXPECT_FLOAT_EQ(puv.texCoordV, 0.75f);

    glcore::Vertex full(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.5f, 0.5f);
    EXPECT_FLOAT_EQ(full.position.z, 3.0f);
    EXPECT_FLOAT_EQ(full.normal.y, 1.0f);
    EXPECT_FLOAT_EQ(full.texCoordU, 0.5f);
}

TEST(MeshDataTest, Validity)
{
    glcore::MeshData empty;
    EXPECT_TRUE(empty.isEmpty());
    EXPECT_FALSE(empty.isValid());
    EXPECT_EQ(empty.vertexCount(), 0u);
    EXPECT_EQ(empty.indexCount(), 0u);
    EXPECT_FLOAT_EQ(empty.emissive.x, 1.0f);

    glcore::MeshData triangle;
    triangle.vertices.push_back(glcore::Vertex(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f));
    triangle.vertices.push_back(glcore::Vertex(glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 0.0f));
    triangle.vertices.push_back(glcore::Vertex(glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 1.0f));
    triangle.indices.push_back(0);
    triangle.indices.push_back(1);
    triangle.indices.push_back(2);
    EXPECT_TRUE(triangle.isValid());
    EXPECT_FALSE(triangle.isEmpty());
    EXPECT_EQ(triangle.vertexCount(), 3u);
    EXPECT_EQ(triangle.indexCount(), 3u);

    // 索引越界应判为无效
    triangle.indices.push_back(3);
    EXPECT_FALSE(triangle.isValid());
}

TEST(MeshDataTest, ComputeNormals)
{
    glcore::MeshData triangle;
    triangle.vertices.push_back(glcore::Vertex(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f));
    triangle.vertices.push_back(glcore::Vertex(glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 0.0f));
    triangle.vertices.push_back(glcore::Vertex(glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 1.0f));
    triangle.indices.push_back(0);
    triangle.indices.push_back(1);
    triangle.indices.push_back(2);

    triangle.computeNormals();
    for (std::size_t i = 0; i < triangle.vertices.size(); ++i)
    {
        // 三角形位于 XY 平面且逆时针，法线应为 +Z
        EXPECT_NEAR(triangle.vertices[i].normal.x, 0.0f, 1.0e-6f);
        EXPECT_NEAR(triangle.vertices[i].normal.y, 0.0f, 1.0e-6f);
        EXPECT_NEAR(triangle.vertices[i].normal.z, 1.0f, 1.0e-6f);
    }
}

// ========== MeshDataBuilder ==========
TEST(MeshDataBuilderTest, BuildInterleaved)
{
    const float data[] = {
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  //
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  //
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f   //
    };

    glcore::MeshDataBuilder builder(8);
    EXPECT_EQ(builder.floatsPerVertex(), 8);
    builder.addVertex(&data[0], 8);
    builder.addVertex(&data[8], 8);
    builder.addVertex(&data[16], 8);
    builder.addTriangle(0, 1, 2);

    const glcore::MeshData mesh = builder.build();
    EXPECT_EQ(mesh.vertexCount(), 3u);
    EXPECT_EQ(mesh.indexCount(), 3u);
    EXPECT_TRUE(mesh.isValid());
    EXPECT_FLOAT_EQ(mesh.vertices[1].position.x, 1.0f);
    EXPECT_FLOAT_EQ(mesh.vertices[2].normal.z, 1.0f);
    EXPECT_FLOAT_EQ(mesh.vertices[1].texCoordU, 1.0f);
    EXPECT_FLOAT_EQ(mesh.vertices[2].texCoordV, 1.0f);
    EXPECT_EQ(mesh.indices[0], 0u);
    EXPECT_EQ(mesh.indices[2], 2u);
}

TEST(MeshDataBuilderTest, PositionOnlyAndAddIndex)
{
    const float positions[] = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    glcore::MeshDataBuilder builder(3);
    builder.addVertex(&positions[0], 3);
    builder.addVertex(&positions[3], 3);
    builder.addIndex(0);
    builder.addIndex(1);

    const glcore::MeshData mesh = builder.build();
    EXPECT_EQ(mesh.vertexCount(), 2u);
    EXPECT_EQ(mesh.indexCount(), 2u);
    EXPECT_TRUE(glcore::isZero(mesh.vertices[0].normal));
}

TEST(MeshDataBuilderTest, InvalidArguments)
{
    EXPECT_THROW(glcore::MeshDataBuilder(0), std::invalid_argument);
    EXPECT_THROW(glcore::MeshDataBuilder(4), std::invalid_argument);
    EXPECT_THROW(glcore::MeshDataBuilder(7), std::invalid_argument);
    EXPECT_THROW(glcore::MeshDataBuilder(-1), std::invalid_argument);

    const float values[] = {0.0f, 0.0f, 0.0f};
    glcore::MeshDataBuilder builder(3);
    EXPECT_THROW(builder.addVertex(&values[0], 2), std::invalid_argument);
    EXPECT_THROW(builder.addVertex(nullptr, 3), std::invalid_argument);
}

TEST(MeshDataBuilderTest, CubeMeshData)
{
    const glcore::MeshData cube = glcore::cubeMeshData();
    EXPECT_EQ(cube.vertexCount(), 36u);
    EXPECT_EQ(cube.indexCount(), 36u);
    EXPECT_TRUE(cube.isValid());
    EXPECT_FALSE(cube.isEmpty());
    EXPECT_FLOAT_EQ(cube.vertices[0].position.x, -0.5f);
    EXPECT_FLOAT_EQ(cube.vertices[0].normal.z, -1.0f);
    EXPECT_EQ(cube.indices[35], 35u);
}

// ========== Camera3D ==========
TEST(Camera3DTest, DefaultMatricesAreFinite)
{
    glcore::Camera3D camera(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -5.0f));
    EXPECT_TRUE(isFinite(camera.viewMatrix()));
    EXPECT_TRUE(isFinite(camera.projectionMatrix()));

    // 默认参数下 fov = 45 度
    const float expectedFocal = 1.0f / std::tan(glcore::degToRad(45.0f) * 0.5f);
    EXPECT_NEAR(camera.projectionMatrix()[1][1], expectedFocal, 1.0e-4f);
}

TEST(Camera3DTest, PositionAndView)
{
    glcore::Camera3D camera(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 3.5f, -5.0f));
    camera.setOrigin(glm::vec3(10.0f, 20.0f, 30.0f));
    camera.update();

    const glm::vec3 expected(10.0f, 23.5f, 25.0f);
    EXPECT_EQ(camera.position(), expected);

    const glm::vec3 inView = transformPoint(camera.viewMatrix(), expected);
    EXPECT_NEAR(inView.x, 0.0f, 1.0e-4f);
    EXPECT_NEAR(inView.y, 0.0f, 1.0e-4f);
    EXPECT_NEAR(inView.z, 0.0f, 1.0e-4f);

    camera.setOffset(glm::vec3(0.0f));
    EXPECT_EQ(camera.position(), glm::vec3(10.0f, 20.0f, 30.0f));
}

TEST(Camera3DTest, AspectRatioAffectsProjection)
{
    glcore::Camera3D camera(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f));

    camera.setAspectRatio(1.0f);
    camera.update();
    const float wide = camera.projectionMatrix()[0][0];

    camera.setAspectRatio(2.0f);
    camera.update();
    const float narrow = camera.projectionMatrix()[0][0];

    EXPECT_NE(wide, narrow);
    EXPECT_NEAR(narrow, wide / 2.0f, 1.0e-5f);
    // aspect 不应影响 y 方向缩放
    EXPECT_NEAR(camera.projectionMatrix()[1][1], wide, 1.0e-5f);
}

TEST(Camera3DTest, YawPitchConvention)
{
    glcore::Camera3D camera(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f));
    camera.setYawPitch(0.0f, 0.0f);

    // yaw = 0、pitch = 0 时朝向 +Z
    EXPECT_NEAR(camera.front().x, 0.0f, 1.0e-6f);
    EXPECT_NEAR(camera.front().y, 0.0f, 1.0e-6f);
    EXPECT_NEAR(camera.front().z, 1.0f, 1.0e-6f);

    const glm::vec3 ray = camera.rayDirection();
    const double alignment = static_cast<double>(glm::dot(glm::normalize(camera.front()), ray));
    EXPECT_NEAR(alignment, 1.0, 1.0e-6);

    // yaw = 90 度时朝向 +X
    camera.setYawPitch(90.0f, 0.0f);
    EXPECT_NEAR(camera.front().x, 1.0f, 1.0e-6f);
    EXPECT_NEAR(camera.front().z, 0.0f, 1.0e-6f);
}

TEST(Camera3DTest, PitchIsClamped)
{
    glcore::Camera3D camera(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f));

    camera.setYawPitch(0.0f, 100.0f);
    EXPECT_FLOAT_EQ(camera.pitch(), glcore::Camera3D::kMaxPitchDegrees);
    EXPECT_NEAR(camera.front().y, std::sin(glcore::degToRad(89.0f)), 1.0e-6f);

    camera.setYawPitch(0.0f, -100.0f);
    EXPECT_FLOAT_EQ(camera.pitch(), -glcore::Camera3D::kMaxPitchDegrees);
    EXPECT_NEAR(camera.front().y, -std::sin(glcore::degToRad(89.0f)), 1.0e-6f);

    // 范围内的 pitch 原样保留
    camera.setYawPitch(0.0f, 30.0f);
    EXPECT_FLOAT_EQ(camera.pitch(), 30.0f);
}

TEST(Camera3DTest, RightAndMovement)
{
    glcore::Camera3D camera(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f));
    camera.setOrigin(glm::vec3(0.0f));
    camera.setYawPitch(0.0f, 0.0f);

    // 面向 +Z、世界上方向 +Y 时，相机右侧为 -X
    EXPECT_NEAR(camera.right().x, -1.0f, 1.0e-6f);
    EXPECT_NEAR(camera.right().y, 0.0f, 1.0e-6f);
    EXPECT_NEAR(camera.right().z, 0.0f, 1.0e-6f);

    camera.moveForward(2.0f);
    EXPECT_NEAR(camera.origin().z, 2.0f, 1.0e-6f);
    EXPECT_NEAR(camera.origin().x, 0.0f, 1.0e-6f);

    camera.moveRight(1.0f);
    EXPECT_NEAR(camera.origin().x, -1.0f, 1.0e-6f);
    EXPECT_NEAR(camera.origin().z, 2.0f, 1.0e-6f);
    EXPECT_NEAR(camera.origin().y, 0.0f, 1.0e-6f);
}

TEST(Camera3DTest, SetFrontAndUpdate)
{
    glcore::Camera3D camera(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f));
    camera.setFront(glm::vec3(0.0f, 0.0f, 10.0f));
    EXPECT_NEAR(camera.front().z, 1.0f, 1.0e-6f);

    // 零向量被忽略，朝向保持不变
    camera.setFront(glm::vec3(0.0f));
    EXPECT_NEAR(camera.front().z, 1.0f, 1.0e-6f);

    camera.setFov(60.0f);
    camera.setNear(0.5f);
    camera.setFar(500.0f);
    camera.update();
    EXPECT_TRUE(isFinite(camera.projectionMatrix()));
    const float expectedFocal = 1.0f / std::tan(glcore::degToRad(60.0f) * 0.5f);
    EXPECT_NEAR(camera.projectionMatrix()[1][1], expectedFocal, 1.0e-4f);
}

// ========== 相机退化输入 ==========
TEST(Camera3DTest, DegenerateWorldUpDoesNotProduceNaN)
{
    // worldUp 与 front 平行时 cross 为零向量，right() 应退化为零向量而不是 NaN
    glcore::Camera3D camera(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f));
    camera.setYawPitch(0.0f, 0.0f);

    EXPECT_TRUE(glcore::isZero(camera.right()));
    EXPECT_FALSE(std::isnan(camera.right().x));
    EXPECT_FALSE(std::isnan(camera.right().y));
    EXPECT_FALSE(std::isnan(camera.right().z));
}
