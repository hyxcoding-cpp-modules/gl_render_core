#include <glm/glm.hpp>
#include <iostream>
#include <string>

#include "glcore/glcore.hpp"

int main()
{
    // ---------- 相机 ----------
    // 向量与矩阵直接用 glm（glm::vec3 / glm::mat4），输出借助 glcore::toString
    glcore::Camera3D camera(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 3.5f, -5.0f));
    camera.setOrigin(glm::vec3(0.0f, 50.0f, 3.0f));
    camera.setFov(45.0f);
    camera.setAspectRatio(800.0f / 600.0f);
    camera.setNear(0.1f);
    camera.setFar(300.0f);
    camera.setYawPitch(0.0f, 0.0f);
    camera.update();

    std::cout << "position   = " << glcore::toString(camera.position()) << '\n';
    std::cout << "front      = " << glcore::toString(camera.front()) << '\n';
    std::cout << "right      = " << glcore::toString(camera.right()) << '\n';
    std::cout << "ray        = " << glcore::toString(camera.rayDirection()) << '\n';
    std::cout << "view:\n" << glcore::toString(camera.viewMatrix()) << '\n';
    std::cout << "projection:\n" << glcore::toString(camera.projectionMatrix()) << '\n';

    // 相机沿朝向前进 10 个单位后再看向右前方
    camera.moveForward(10.0f);
    camera.setYawPitch(45.0f, -20.0f);
    camera.update();
    std::cout << "after move = " << glcore::toString(camera.position())
              << " front = " << glcore::toString(camera.front()) << '\n';

    // ---------- 立方体网格数据 ----------
    const glcore::MeshData cube = glcore::cubeMeshData();
    std::cout << "cube vertices = " << cube.vertexCount() << " , indices = " << cube.indexCount()
              << " , valid = " << (cube.isValid() ? "true" : "false") << '\n';
    std::cout << "cube emissive = " << glcore::toString(cube.emissive) << '\n';

    // ---------- uniform 位置缓存 ----------
    glcore::ShaderUniformCache cache;
    cache.setLocation("model", 0);
    cache.setLocation("view", 1);
    cache.setLocation("projection", 2);
    cache.setLocation("light.color", 3);
    std::cout << "uniform cache size = " << cache.size() << '\n';
    const std::vector<std::string> names = cache.names();
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        std::cout << "  " << names[i] << " -> " << cache.getLocation(names[i]) << '\n';
    }
    std::cout << "missing uniform -> " << cache.getLocation("emissive") << '\n';

    // ---------- 注释剥离 ----------
    const std::string raw =
        "// 头部注释\n"
        "uniform vec3 lightColor; /* 内联块注释 */\n"
        "const char* url = \"http://example.com\";\n";
    std::cout << "--- stripped source ---\n" << glcore::stripComments(raw) << "--- end ---\n";

    // ---------- 顶点数据构造 ----------
    const float positions[] = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    glcore::MeshDataBuilder builder(3);
    builder.addVertex(&positions[0], 3);
    builder.addVertex(&positions[3], 3);
    builder.addVertex(&positions[6], 3);
    builder.addTriangle(0, 1, 2);
    const glcore::MeshData triangle = builder.build();
    std::cout << "triangle vertices = " << triangle.vertexCount()
              << " , indices = " << triangle.indexCount() << '\n';

    return 0;
}
