# 架构说明

## 分层

```
glcore.hpp                核心层聚合入口
  ├── util.hpp            无第三方依赖（仅 <cmath> 级别的常量与工具）
  ├── math.hpp            依赖 util.hpp + glm：isZero / normalizedOrZero / toString
  ├── shader_source.hpp   仅标准库
  ├── mesh_data.hpp       依赖 glm（Vertex / MeshData 直接用 glm::vec3）
  └── camera3d.hpp        依赖 math.hpp + glm（glm::vec3 / glm::mat4）

glcore/gl/                OpenGL 后端（默认关闭，由 GLRENDER_ENABLE_GL 控制）
  ├── shader_program.hpp  依赖 shader_source.hpp + glm + glad
  ├── gpu_mesh.hpp        依赖 mesh_data.hpp、shader_program.hpp + glad
  ├── texture_loader.hpp  依赖 glad + stb_image
  ├── model_loader.hpp    依赖 gpu_mesh.hpp + assimp
  └── skybox.hpp          依赖 texture_loader.hpp、camera3d.hpp
```

依赖关系为单向、无环。核心层不引用 `glcore/gl/` 下的任何内容，也不引用其它模块（M6/M7/…）的头文件。

**向量与矩阵运算全部由 glm 提供**（`glm::vec3` / `glm::mat4`），核心层不再自带 `Vec3` / `Mat4`。因此核心层从「零第三方依赖」变成「只依赖 glm（仅头文件）」。唯一新增的核心层头文件 `math.hpp` 不含任何数学实现，只补 glm 缺失的三件小事。

## 头文件 / 源文件划分

| 类型 | 位置 | 原因 |
|------|------|------|
| `isZero` / `normalizedOrZero` | `math.hpp` 内联 | 极短，便于内联优化 |
| `toString(vec3)` / `toString(mat4)` | `src/math.cpp` | 含 `std::ostringstream`，适合独立编译单元 |
| `ShaderSource`、`readFile`、`stripComments` | `src/shader_source.cpp` | 非模板，含文件 IO 与逐字符扫描 |
| `ShaderUniformCache` | `src/shader_source.cpp` | 与着色器源码同属「着色器预处理」关注点 |
| `MeshData` / `MeshDataBuilder` / `cubeMeshData` | `src/mesh_data.cpp` | 非模板，含内置顶点表 |
| `Camera3D` | `src/camera3d.cpp` | 非模板，状态机 + 矩阵重算 |
| GL 后端全部类 | `src/gl/*.cpp` | 只在 `GLRENDER_ENABLE_GL=ON` 时参与编译 |
| glad / stb_image | `third_party/` → 目标 `gl_render_core_vendor` | vendored 源码，不套用本工程的告警集 |

## 关键决策

### 1. 核心层与后端彻底分离

原实现里 `Mesh` 的构造函数直接 `glGenVertexArrays`，`Shader` 的构造函数直接读文件并 `glCompileShader`——任何一处都离不开 GL 上下文，导致这些类型**完全无法单元测试**。

本模块把「数据」和「上传」拆开：

```
main.cpp 的 float vertices[]  →  MeshDataBuilder  →  MeshData  →（GL 后端）GpuMesh::upload
shader 文件                    →  fromFiles        →  ShaderSource →（GL 后端）ShaderProgram::compile
```

`MeshData` / `ShaderSource` / `ShaderUniformCache` 都是纯数据结构，因此可以在没有任何图形库的机器上断言其正确性（`isValid()`、`stripComments()`、位置缓存命中与排序等）。

### 2. 数学统一用 glm

原先核心层自带一份 `Vec3` + 列主序 `Mat4`（约 300 行），GL 后端里却还要再和 glm 互转（`Skybox` 用 `glm::make_mat4` / `glm::mat3` 截断、`ModelLoader` 用 `glm::vec3` 做缩放与颜色转换）。两套类型并存带来的是：

- 转换代码本身没有测试覆盖，只是「顺手验证了列主序一致」；
- 公开头文件里的 `Vec3` / `Mat4` 与 glm 的 API 不完全一样（`at(row, col)`、`transformPoint`、`length()` 返回 `double`），使用者要记两套；
- 每次要在后端用到 glm 特性都得再写一次桥接。

现在**删掉自带实现，全部改用 glm**：

- `Vec3` → `glm::vec3`，`Mat4` → `glm::mat4`；
- `Mat4::perspective` / `Mat4::lookAt` / `Mat4::translation` / `Mat4::scale` → `glm::perspective` / `glm::lookAt` / `glm::translate` / `glm::scale`；
- `Mat4::operator*`、`data()` → glm 原生运算符与 `glm::value_ptr()`；
- `Vec3::dot` / `cross` / `normalized` / `length` → `glm::dot` / `glm::cross` / `glm::normalize` / `glm::length`；
- `Mat4::transformPoint` 与 `Vec3::isZero` 属于 glm 没有的便利接口：前者只出现在测试里（测试内用 `glm::vec4` 自行复现），后者放进 `math.hpp`。

注意 glm 的索引是 `m[列][行]`，等价于原 `at(row, col)`；移植时不要按行主序读。

### 3. 相机朝向约定为 `+Z`

原 `Camera::m_baseFront = glm::vec3(0, 0, 1)`，`update()` 里 `glm::lookAt(pos, pos + front, up)`。本模块沿用这一约定：`yaw = 0`、`pitch = 0` 时 `front()` 为 `(0, 0, 1)`。

由此推出的一个**容易误判的结论**：面向 `+Z`、世界上方向为 `+Y` 时，相机右侧是 `-X`（右手系）。`right()` 的实现为 `normalizedOrZero(glm::cross(front, worldUp))`，与 `glm::lookAt` 内部使用的 `side = cross(forward, up)` 完全一致。测试 `Camera3DTest.RightAndMovement` 固定了这一行为。

### 4. `setYawPitch` 是唯一会覆盖 `setFront` 的入口

`setFront` 与 `setYawPitch` 都写 `m_front`，后调用者生效。`setFront` 会把输入归一化，并忽略零向量（否则 `glm::lookAt` 会退化成 NaN 矩阵）。`setYawPitch` 会把 `pitch` 钳制到 `[-89, 89]`，避免 ±90° 时 `right()` 退化。

### 5. 零向量安全由 `math.hpp` 兜住

`glm::normalize` 对零向量会返回 NaN。本模块有三处会产生退化向量：

| 位置 | 退化原因 | 处理 |
|------|----------|------|
| `Camera3D::setFront` | 调用方传了零向量 | 直接忽略，保持原朝向 |
| `Camera3D::right` | `front` 与 `worldUp` 平行 | `normalizedOrZero` → 零向量 |
| `MeshData::computeNormals` | 退化三角形面积为 0 | `normalizedOrZero` → 零向量 |

`Camera3DTest.DegenerateWorldUpDoesNotProduceNaN` 固定了第 2 条。

### 6. `stripComments` 必须理解字符串字面量

原项目里 `Shader` 只是把文件内容原样喂给 GL。做注释剥离时如果只按 `//`、`/*` 做文本替换，会把 `"http://example.com"` 这类字面量切坏。因此 `stripComments` 维护「是否在双/单引号内」与「反斜杠转义」两个状态，只在字符串外识别注释。具体边界行为写在 `shader_source.hpp` 的注释里。

### 7. 错误处理：文件与 GL 用返回值，编程错误用异常

- `readFile` / `fromFiles`：失败返回 `false` / 不完整的 `ShaderSource`，不抛异常（运行时环境问题）。
- `ShaderProgram::compile`：失败返回 `false` 并写 `lastError()`（GL 运行时问题）。
- `MeshDataBuilder`：`floatsPerVertex` 非法或 `addVertex` 数量不匹配时抛 `std::invalid_argument`（调用方的编程错误，属于不可恢复）。

原 `Shader` 在编译失败时 `return` 出构造函数，留下一个 `m_id` 未初始化的对象；原 `Model::processMesh` 里 `switch (nrc)` 缺 `default`，`format` 可能是未初始化值。这两类问题在本模块中都已消除。

### 8. GL 后端默认关闭，但依赖已全部接入

`GLRENDER_ENABLE_GL` 默认 `OFF`：后端需要 GL 上下文才能真正跑起来，单元测试无从谈起；核心层与全部单元测试在只装 glm 的环境里就能 100% 通过。

依赖不再「尚未接入」——`cmake/ThirdParty.cmake` 已经能处理全部五个依赖：

| 依赖 | 来源 | 定位方式 |
|------|------|----------|
| glad / stb_image | 仓库内 `third_party/` | 直接引用（存在性校验） |
| glm | `D:/third_party/glm`，缺失下载 `glm-1.0.3.zip` | 标志文件 `glm/glm.hpp` |
| glfw | `D:/third_party/OpenGL/glfw-3.4`，缺失下载 `glfw-3.4.zip` | 标志文件 `CMakeLists.txt` + `include/GLFW/glfw3.h`，命中后 `add_subdirectory` |
| assimp | `D:/third_party/assimp-5.4.3`，缺失下载 `v5.4.3.zip` | 标志文件 `include/assimp/scene.h`；有预编译库就直接链接，否则 `add_subdirectory` |
| 系统 OpenGL | `find_package(OpenGL REQUIRED)` | — |

下载后解压出来的层级**不写死**：`_third_party_find_root()` 会从写死路径开始、逐层向下递归查找「标志文件齐全的目录」，找到的那一层就是包根。这样上游改了压缩包的目录结构也不会让配置失败。
