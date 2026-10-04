# 决策记录

按时间倒序记录本模块的重要技术决策。

---

## D-010 用 glm 替换核心层自带的 `Vec3` / `Mat4`

- **日期**：2026-10-04
- **决策**：**删除** `include/glcore/vec3.hpp`、`include/glcore/mat4.hpp`、`src/vec3.cpp`、`src/mat4.cpp`，核心层与 GL 后端一律直接使用 `glm::vec3` / `glm::mat4`。核心层新增 `include/glcore/math.hpp` + `src/math.cpp`，只提供 glm 没有的三件小事：`isZero`、`normalizedOrZero`（零向量安全的归一化）、`toString`。
- **取代**：**D-001**（自带 `Vec3` / `Mat4`，核心层不依赖 glm）与 **D-003**（修正自带 `Mat4::perspective` 的深度项）——两份实现都已不存在，D-003 修出来的公式现在由 glm 提供。
- **理由**：
  1. 两套类型并存是有成本的。GL 后端本来就要和 glm 打交道（`Skybox` 用 `glm::make_mat4` / `glm::mat3` 截断 view、`ModelLoader` 用 `glm::vec3` 做缩放与颜色转换），自带的 `Vec3` / `Mat4` 只让这些地方多了一层转换，而转换本身没有测试覆盖。
  2. 自带的 API 与 glm 不完全一样（`at(row, col)`、`transformPoint`、`length()` 返回 `double`、`isZero`），使用者要同时记两套语义。
  3. 原项目 `Ark_Like` 本来就是用 glm 的，本模块的定位是「提炼可复用底座」，保留一份自制数学库属于额外的维护面。
- **代价**：
  1. 核心层从「零第三方依赖」变成「依赖 glm（仅头文件）」，因此**默认构建也必须先取到 glm**。`cmake/ThirdParty.cmake` 在 `include()` 时就完成 glm 定位，CI 的 lint job 也需要它（已改为把第三方目录放到 workspace 下）。
  2. glm 内部有 `-Wsign-conversion` 之类的告警，必须用 `SYSTEM` 方式包含（`target_include_directories(gl_render_core SYSTEM PUBLIC ...)`），否则会破坏本工程 0 warning 的目标。
  3. `install(EXPORT)` 导出的目标只带 `BUILD_INTERFACE` 下的 glm 目录，**安装后使用方需自行提供 glm**。
  4. glm 的索引是 `m[列][行]`，与原来的 `at(row, col)` 互为转置。移植测试时踩过一次（`PerspectiveAspectNearFar` 里 `[2][3]` / `[3][2]` 写反），已在测试里加了注释说明。
- **验证**：MinGW + TDM-GCC 10.3.0，默认构建 0 warning，31 项单元测试全过，示例程序正常输出；`GLRENDER_ENABLE_GL=ON` 的后端也能 0 warning 编译链接。

## D-009 可选后端的第三方依赖全部接入 `ThirdParty.cmake`

- **日期**：2026-10-04
- **决策**：把 glad / stb / glm / glfw / assimp / 系统 OpenGL 全部接入 `cmake/ThirdParty.cmake`，`GLRENDER_GLAD_SOURCE` / `GLRENDER_GL_INCLUDE_DIRS` / `GLRENDER_GL_LIBRARIES` 不再为空。
- **分工**：
  - **glad / stb** 是**仓库内 vendored 源码**（`third_party/glad`、`third_party/stb`），随仓库提交、不下载，编译成独立目标 `gl_render_core_vendor`（不套用本工程的严格告警集，`glad.c` 是生成代码）。为此 `.gitignore` 从 `third_party/*` + `!README.md` 放宽为额外放行 `glad/`、`stb/`，否则 CI 上 `checkout` 拿不到它们。
  - **glm** 本地写死 `D:/third_party/glm`，缺失下载 `glm-1.0.3.zip`。
  - **glfw** 本地写死 `D:/third_party/OpenGL/glfw-3.4`，缺失下载 `glfw-3.4.zip`，命中后调用它自己的 `CMakeLists.txt`。
  - **assimp** 本地写死 `D:/third_party/assimp-5.4.3`；`build/lib` 下有预编译库（本机是 MinGW 产物 `libassimp.dll.a`）就直接链接，否则下载 `v5.4.3.zip` 并调用它自己的 `CMakeLists.txt`。关闭 `ASSIMP_BUILD_TESTS`（它需要 `contrib/googletest`，本地没有）。
- **下载后不写死层级**：沿用 audio_manager 里验证过的做法——`_third_party_find_root()` 从写死路径开始逐层递归查找「标志文件齐全的目录」，找到的那一层就是包根。上游改了压缩包目录结构也不会让配置失败。
- **代价**：
  1. assimp 从源码构建时，它自己的 `CMakeLists.txt` 会把 `CMAKE_CXX_STANDARD` 设为 17。因为 `add_subdirectory` 会新建作用域，这个改动**只影响 assimp 自己的子树**，本工程仍是 C++14；已用本地预编译库路径与源码路径分别验证过配置阶段的行为。
  2. CI 新增 `build-gl` job 专门编译后端。后端本身没有单元测试（需要 GL 上下文），该 job 只验证「能配置 + 能编译链接」。
  3. 顺带修掉了 `ShaderProgram::uniformBlockIndex` 里一处 `GLuint → int` 的 `-Wsign-conversion` 告警——这段代码此前从未被编译过。

## D-008 OpenGL 后端默认关闭

- **日期**：2026-10-03
- **决策**：新增 CMake 选项 `GLRENDER_ENABLE_GL`，默认 `OFF`。开启时才使用 glad / glm / assimp / stb，缺失即 `FATAL_ERROR`。
- **理由**：后端需要 GL 上下文才能跑起来，无法在无显卡的 CI 上做单元测试。把后端隔离到选项之后，核心层与全部单元测试可以在只装 glm 的环境里 100% 通过。
- **代价**：后端代码（`src/gl/*.cpp`）没有单元测试覆盖，只能保证能编译链接。（D-009 起依赖已全部接入，不再有「尚未接入」的问题。）

## D-007 保持 C++14 标准

- **日期**：2026-10-03
- **决策**：模块仅使用 C++11/14 特性，`CMAKE_CXX_STANDARD` 设为 14。**未使用任何 C++17 及以上特性**，因此无需上调标准。
- **理由**：原 `Ark_Like` 以 C++17 编译，但本模块定位为可复用底座，降低标准后可被仍以 C++11/14 编译的其它项目直接引用。
- **具体规避**：
  - 不用 `std::optional`，`readFile` / `compile` 改用 `bool` 返回值 + `lastError()`；
  - 不用 `std::string_view`，接口统一用 `const std::string&`；
  - 不用内联变量，`ShaderSource` 相关的自由常量（如 `kInvalidUniformLocation`）以 `constexpr` 定义在头文件里（`constexpr` 隐含内部链接，无 ODR 问题）；
  - `Camera3D` 的 `static constexpr float` 成员在 `src/camera3d.cpp` 中补了类外定义，保证被 odr-use（例如取地址）时能链接通过；
  - 不用结构化绑定与 `if constexpr`，遍历容器统一写 `for (std::size_t i = 0; ...)` 或显式迭代器。
- **补充（D-010 之后）**：glm 1.0.3 支持 C++14。本模块用到的接口（`translate` / `scale` / `perspective` / `lookAt` / `value_ptr` / `mat3(mat4)` / 向量比较运算符）在 `-std=c++14` 下均已实测可用，且配合 `-isystem` 可做到 0 warning。

## D-006 相机朝向沿用 `+Z`

- **日期**：2026-10-03
- **决策**：`yaw = 0`、`pitch = 0` 时 `front()` 为 `+Z`，与原 `Camera::m_baseFront = glm::vec3(0, 0, 1)` 一致。
- **理由**：保持与原始项目行为一致，迁移时不需要改动调用方的 yaw 初始值（原 `main.cpp` 里 `yaw = 270.0f`）。
- **代价**：与 OpenGL 惯例（相机看向 `-Z`）相反，也与多数教程相反。为避免误用，约定已写进 `camera3d.hpp` 的头文件注释，并有测试固定行为。由此推出「面向 `+Z`、`+Y` 为上时右侧为 `-X`」，`right()` 用 `normalizedOrZero(cross(front, worldUp))` 实现，与 `glm::lookAt` 内部使用的 `side = cross(forward, up)` 完全一致。

## D-005 `MeshDataBuilder` 用异常，文件 / GL 用返回值

- **日期**：2026-10-03
- **决策**：`MeshDataBuilder` 的构造参数非法、`addVertex` 的 `count` 不匹配、`data` 为空指针时抛 `std::invalid_argument`；而 `readFile`、`fromFiles`、`ShaderProgram::compile`、`ModelLoader::load` 失败时返回 `false`。
- **理由**：前者是调用方的编程错误，静默返回空网格会让问题在很久以后才暴露；后者是运行时环境问题（文件不存在、显卡不支持），游戏主循环需要能优雅降级。
- **代价**：`MeshDataBuilder` 在热路径上不可用。实际使用中它只用于一次性构造静态网格，可接受。

## D-004 `stripComments` 识别字符串字面量

- **日期**：2026-10-03
- **决策**：剥离注释时维护「双引号内 / 单引号内 / 反斜杠转义」状态，只在字符串外把 `//`、`/*` 当注释。
- **理由**：着色器源码里可能出现 `"http://..."` 这类字面量；朴素替换会破坏源码。原项目没有这个函数，属于提炼过程中新增的能力。
- **边界行为**（写在 `shader_source.hpp` 注释里）：
  - 行注释删除后保留换行，避免影响 `#line` 与报错行号；
  - 块注释整体删除，内部换行一并删除；
  - 未闭合的块注释视为延伸到末尾；
  - 未闭合的字符串字面量原样保留到末尾。

## D-003 修正 `perspective` 的深度项 —— 已被 D-010 取代

- **日期**：2026-10-03
- **决策**：`Mat4::perspective` 采用 `m[2][2] = -(far + near) / (far - near)`、`m[2][3] = -(2 * far * near) / (far - near)`、`m[3][2] = -1`。
- **理由**：初次实现时把 `m[2][3]` 与 `m[3][2]` 写反、并把深度项写成正值，导致近平面映射到 NDC `z = +1`、远平面映射到 `z = -1`，深度测试会整体反向。补上「近平面 → NDC -1、远平面 → NDC +1」的测试后暴露并修正。
- **现状**：`Mat4` 已随 D-010 删除，透视矩阵改由 `glm::perspective` 提供（公式一致）。这条测试仍然保留在 `Mat4Test.PerspectiveAspectNearFar` 中，用来固定「glm 的透视矩阵与我们的深度约定一致」。

## D-002 用 `MeshData` 承载 CPU 侧数据

- **日期**：2026-10-03
- **决策**：新增 `MeshData`（`vertices` / `indices` / `emissive`）作为「构造」与「上传」之间的中间表示；原 `Mesh` 类被拆为 `MeshData`（核心层）+ `GpuMesh`（GL 后端）。
- **理由**：原 `Mesh` 构造函数里直接调 GL，无法在无 GL 环境下测试 `isValid()`、法线重算、索引越界等纯数据逻辑。拆开后这些逻辑可以独立断言。
- **代价**：多一层类型转换（`MeshData` → `GpuMesh`），但换来了可测试性与后端可替换性。

## D-001 自带 `Vec3` / `Mat4`，核心层不依赖 glm —— 已废弃，被 D-010 取代

- **日期**：2026-10-03
- **原决策**：核心层自带 `Vec3` 与列主序 `Mat4`，`include/glcore/` 下不出现 `glm` 头文件。
- **原理由**：glm 是仅头文件库，但它把核心数学与「渲染模块」耦合在一起；本模块的测试需要在没有 glm 的环境里跑通。自带的实现只覆盖实际用到的能力（向量四则运算、`dot`/`cross`/`normalized`、单位阵/平移/缩放/透视/`lookAt`、点变换），约 300 行，维护成本可控。
- **废弃原因**：见 **D-010**。两套类型并存带来的桥接代码与 API 差异成本，高于「核心层不依赖 glm」的收益；而原项目本来就以 glm 为数学层。
- **现状**：`include/glcore/vec3.hpp`、`include/glcore/mat4.hpp`、`src/vec3.cpp`、`src/mat4.cpp` 已删除，全仓库不再出现 `glcore::Vec3` / `glcore::Mat4`。
