# gl_render_core

OpenGL 渲染基础模块。提供与图形后端无关的数学与数据层（**glm 数学** / 着色器源码与 uniform 缓存 / 网格数据 / 相机），以及一套**默认关闭**的 OpenGL 后端（着色器程序、GPU 网格、纹理与模型加载、天空盒）。

## 模块定位

从 `Ark_Like` 项目里反复出现、且被 `Shader` / `Mesh` / `Model` / `Camera` / `SkyBox` / `TextureLoader` / `Crosshair` 共用的一层代码提炼而成。原实现把所有逻辑都绑死在 glad + glm + assimp 上，任何一个文件都必须在有 OpenGL 上下文的环境里才能编译。本模块把它拆成两层：

| 层 | 依赖 | 是否默认构建 |
|----|------|--------------|
| 核心层 `glcore/` | glm（仅头文件） | 是 |
| OpenGL 后端 `glcore/gl/` | glad / glm / assimp / glfw / stb_image / 系统 OpenGL | 否（`GLRENDER_ENABLE_GL=OFF`） |

向量与矩阵**不再自带实现**，全部使用 glm（`glm::vec3` / `glm::mat4`）。核心层只保留极少量的补充工具（零向量判定、零向量安全的归一化、文本输出），见 `include/glcore/math.hpp`。

原实现来源：

| 来源 | 原始内容 |
|------|----------|
| `Ark_Like/src/shader.hpp` `.cpp` | 着色器编译链接、`setInt/setFloat/setVec3/setMat4`、`uniformBlockBinding`、`findLoc` / `getUBI` / `std::map<std::string,int> m_cache` |
| `Ark_Like/src/mesh.hpp` `.cpp` | `struct Vertex` / `struct Texture`、VAO/VBO/EBO、`setupMesh`、`draw`、`setEmissive` |
| `Ark_Like/src/model.hpp` `.cpp` | Assimp 加载、`processNode` / `processMesh` / `loadMaterialTextures`、内嵌纹理、按路径去重的 `m_textureLoaded` |
| `Ark_Like/src/textureLoader.hpp` `.cpp` | 2D 纹理与立方体贴图加载 |
| `Ark_Like/src/skybox.hpp` `.cpp` | 六面天空盒 |
| `Ark_Like/src/camera.hpp` `.cpp` | 相机状态、`getView` / `getProject` / `getRay` / `update` |
| `Ark_Like/src/main.cpp` | 手写的立方体 `float vertices[]`（36 顶点 × 8 float） |
| `Ark_Like/src/crosshair.hpp` `.cpp` | 准星（与相机/着色器强耦合，未纳入本模块） |

## 目录结构

```
include/glcore/
  util.hpp            数学常量、clampValue / degToRad / radToDeg / approxEqual
  math.hpp            glm 补充工具：isZero / normalizedOrZero / toString
  shader_source.hpp   ShaderSource、readFile / fromFiles / fromStrings / stripComments、ShaderUniformCache
  mesh_data.hpp       Vertex、TextureInfo、MeshData、MeshDataBuilder、cubeMeshData
  camera3d.hpp        Camera3D
  glcore.hpp          聚合头文件（只含核心层）
  gl/                 OpenGL 后端（默认不编译）
    shader_program.hpp
    gpu_mesh.hpp
    texture_loader.hpp
    model_loader.hpp
    skybox.hpp
src/
  math.cpp            vec3 / mat4 的文本输出
  shader_source.cpp   文件读取、注释剥离、uniform 位置缓存
  mesh_data.cpp       法线重算、扁平数组构造、内置立方体
  camera3d.cpp        相机状态与矩阵更新
  gl/                 后端实现（默认不编译）
third_party/
  glad/               vendored：OpenGL 函数加载器（随仓库提交，不下载）
  stb/                vendored：stb_image 及其实现包装（随仓库提交，不下载）
```

## 快速开始

```cpp
#include <glm/glm.hpp>

#include "glcore/glcore.hpp"

// 相机：yaw = 0、pitch = 0 时朝向 +Z
glcore::Camera3D camera(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 3.5f, -5.0f));
camera.setOrigin(glm::vec3(0.0f, 50.0f, 3.0f));
camera.setAspectRatio(800.0f / 600.0f);
camera.setYawPitch(0.0f, 0.0f);
camera.update();

// glm 为列主序，可直接给 glUniformMatrix4fv(..., GL_FALSE, ptr)
const float* viewPtr = glm::value_ptr(camera.viewMatrix());
const float* projPtr = glm::value_ptr(camera.projectionMatrix());

// 网格数据：36 个顶点、8 float/顶点
const glcore::MeshData cube = glcore::cubeMeshData();
bool ok = cube.isValid();                                // true

// 着色器源码
glcore::ShaderSource source = glcore::fromFiles("vs.glsl", "fs.glsl");
std::string cleaned = glcore::stripComments(source.vertex);

// uniform 位置缓存
glcore::ShaderUniformCache cache;
cache.setLocation("model", 0);
int loc = cache.getLocation("model");                    // 0

// 零向量安全的归一化（glm::normalize 对零向量会产生 NaN）
const glm::vec3 safe = glcore::normalizedOrZero(glm::vec3(0.0f, 0.0f, 0.0f));
```

## 设计要点

- **数学全部交给 glm**：核心层不再自带 `Vec3` / `Mat4`，公开头文件直接使用 `glm::vec3` / `glm::mat4`。glm 是列主序、与 OpenGL 内存布局一致，`glm::value_ptr()` 可直接传给 `glUniformMatrix4fv(..., GL_FALSE, ptr)`。
- **只补 glm 缺的那一点点**：`include/glcore/math.hpp` 里的 `isZero` / `normalizedOrZero` / `toString` 不实现任何数学逻辑，只解决 glm 没有对应接口、而本模块反复需要的问题（零向量归一化会产生 NaN、glm 没有 `operator<<`）。
- **零向量安全**：`normalizedOrZero()` 对零向量返回零向量；`Camera3D::right()`、`MeshData::computeNormals()` 都依赖这一约定，退化输入不会得到 NaN。
- **后端解耦**：核心层只产出 `MeshData` / `ShaderSource` / 矩阵，不知道 GL 的存在。所有 GL 调用集中在 `glcore/gl/` 下，用 `GLRENDER_ENABLE_GL` 控制。
- **相机朝向约定写进头文件**：`yaw = 0`、`pitch = 0` 时 `front()` 为 `+Z`（沿用原 `Camera::m_baseFront = (0,0,1)`）；`pitch` 钳制到 `[-89°, 89°]`。
- **错误不抛异常**：文件读取失败返回 `false`，着色器编译失败返回 `false` 并写 `lastError()`。只有编程错误（如 `MeshDataBuilder` 参数非法）才抛 `std::invalid_argument`。

## 语言标准

**C++14**。`CMAKE_CXX_STANDARD` 为 14，未使用 `if constexpr`、结构化绑定、`std::optional`、`std::string_view`、`std::filesystem`、内联变量等 C++17 特性。glm 1.0.3 支持 C++14，本模块用到的接口（`translate` / `scale` / `perspective` / `lookAt` / `value_ptr` / `mat3(mat4)`）在 C++14 下均可正常编译。原 `Ark_Like` 使用 C++17，但本模块作为可复用底座降到 C++14，以便被仍以 C++11/14 编译的项目直接引用。详见 `docs/DECISIONS.md`。

## 构建与测试

默认构建（**只需要 glm**）：

```bash
cmake -B build/debug -G "MinGW Makefiles" \
      -DCMAKE_C_COMPILER=D:/TDMGCC/bin/gcc.exe \
      -DCMAKE_CXX_COMPILER=D:/TDMGCC/bin/g++.exe \
      -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
cmake --build build/debug
ctest --test-dir build/debug --output-on-failure
./build/debug/example_gl_render_core.exe
```

也可直接使用预设：`cmake --preset debug && cmake --build --preset debug`。

## 可选后端：OpenGL

```bash
# 用预设（Release，只构建后端）
cmake --preset gl && cmake --build --preset gl

# 或者手动指定
cmake -B build/gl -G "MinGW Makefiles" \
      -DCMAKE_BUILD_TYPE=Release -DGLRENDER_ENABLE_GL=ON
cmake --build build/gl --target gl_render_core_gl
```

开启后需要的依赖，全部由 `cmake/ThirdParty.cmake` 处理（**本地写死路径优先，落空才下载**）：

| 库 | 本地期望位置（第三方根目录 `D:/third_party`） | 缺失时的下载地址 | 用途 |
|----|-----------------------------------------------|------------------|------|
| 系统 OpenGL | 由 `find_package(OpenGL REQUIRED)` 提供 | — | 绘制 |
| glad | **仓库内** `third_party/glad/`（源码，随仓库提交） | 不下载 | 函数加载 |
| stb_image | **仓库内** `third_party/stb/`（源码，随仓库提交） | 不下载 | 图像解码 |
| glm | `D:/third_party/glm`（含 `glm/glm.hpp`） | `glm-1.0.3.zip` | 数学 |
| glfw | `D:/third_party/OpenGL/glfw-3.4`（含 `CMakeLists.txt`） | `glfw-3.4.zip`（源码，调用其 CMakeLists） | 窗口 / 上下文 |
| assimp | `D:/third_party/assimp-5.4.3`（`build/lib` 下有预编译库则直接用） | `v5.4.3.zip`（源码，调用其 CMakeLists） | 模型加载 |

下载后解压出来的目录名**不写死**：`cmake/ThirdParty.cmake` 会在第三方目录下递归查找「含标志文件的目录」并据此定位包根，上游改了目录层级也不会让配置失败。

说明：

- glad 与 stb 是**仓库内的 vendored 源码**，不参与下载；它们被编译成独立目标 `gl_render_core_vendor`，不套用本工程的严格告警集（glad.c 是生成代码）。
- assimp 本地包里的 `build/` 已带 MinGW 预编译产物（`lib/libassimp.dll.a` + `bin/libassimp-5.dll`），命中时直接链接，不重复编译源码；只有找不到预编译库时才下载源码并调用它自己的 `CMakeLists.txt`（它会把自身子树设为 C++17，不影响本工程的 C++14）。
- Windows 下构建完成后会把 assimp 的 dll 拷到产物目录。

## 依赖

| 依赖 | 何时需要 | 说明 |
|------|----------|------|
| glm | **始终** | 核心层的公开依赖，`camera3d.hpp` / `mesh_data.hpp` 直接包含 `<glm/glm.hpp>`。以 `SYSTEM` 方式包含，避免 glm 内部的告警污染本工程的 0 warning 目标 |
| googletest | `BUILD_TESTS=ON` | 默认取 `D:/third_party/googletest-1.18.0`，缺失时自动下载 |
| glad / stb_image | `GLRENDER_ENABLE_GL=ON` | 仓库内 vendored 源码 |
| glfw / assimp / 系统 OpenGL | `GLRENDER_ENABLE_GL=ON` | 本地优先，缺失时自动下载 |

`install(EXPORT)` 导出的目标只带 `BUILD_INTERFACE` 下的 glm 包含目录，因此**安装后使用方需要自行提供 glm**。
