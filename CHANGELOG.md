# 更新日志

本文件记录本模块的重要变更。

格式参考 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

---

## [0.2.0] - 2026-10-04

### 变更（不兼容）

- **删除核心层自带的 `Vec3` / `Mat4`**，向量与矩阵全部改用 glm（`glm::vec3` / `glm::mat4`）。
  删除的文件：`include/glcore/vec3.hpp`、`include/glcore/mat4.hpp`、`src/vec3.cpp`、`src/mat4.cpp`。
  全仓库不再出现 `glcore::Vec3` / `glcore::Mat4`，公开 API 与调用方需要同步改造。
- 对应的 API 迁移：
  | 原接口 | 现在 |
  |--------|------|
  | `Vec3` / `Mat4` | `glm::vec3` / `glm::mat4` |
  | `Vec3::dot` / `cross` | `glm::dot` / `glm::cross` |
  | `Vec3::normalized()` | `glm::normalize()`，退化输入用 `glcore::normalizedOrZero()` |
  | `Vec3::length()`（返回 `double`） | `glm::length()`（返回 `float`） |
  | `Vec3::lengthSquared()` | `glm::dot(v, v)` |
  | `Mat4::identity()` | `glm::mat4(1.0f)` |
  | `Mat4::translation` / `scale` | `glm::translate` / `glm::scale` |
  | `Mat4::perspective` / `lookAt` | `glm::perspective` / `glm::lookAt` |
  | `Mat4::data()` | `glm::value_ptr()` |
  | `Mat4::at(row, col)` | `m[col][row]`（glm 为列主序，注意行列互换） |
  | `Mat4::transformPoint` | 核心层不再提供；需要时用 `glm::vec4` 自行做 w 归一化 |
  | `operator<<` | `glcore::toString()` |
- `Camera3D`、`Vertex`、`MeshData`、`GpuMesh`、`ShaderProgram`、`ModelLoader`、`Skybox` 的公开签名同步改为 glm 类型。
- `CMAKE_CXX_STANDARD` 仍为 **14**（glm 1.0.3 支持 C++14）。

### 新增

- `include/glcore/math.hpp` + `src/math.cpp`：对 glm 的少量补充，**不含任何数学实现**。
  - `isZero(const glm::vec3&)`：三分量近似为零判定；
  - `normalizedOrZero(const glm::vec3&)`：零向量安全的归一化（`glm::normalize` 对零向量会返回 NaN）；
  - `toString(const glm::vec3&)` / `toString(const glm::mat4&)`：日志与示例用的文本输出。
- `src/gl/*` 与 `third_party/glad`、`third_party/stb` 全部纳入版本库，CI 上可直接构建 GL 后端。
- 新增测试 `MathTest.IsZeroAndSafeNormalize`、`MathTest.ToStringVec3AndMat4`、
  `Camera3DTest.DegenerateWorldUpDoesNotProduceNaN`。

### 构建

- `cmake/ThirdParty.cmake` 接入可选后端的全部依赖：glm / glfw / assimp / glad / stb / 系统 OpenGL，
  `GLRENDER_GLAD_SOURCE` / `GLRENDER_GL_INCLUDE_DIRS` / `GLRENDER_GL_LIBRARIES` 不再为空。
  - 本地路径写死，落空才下载官方包；
  - 下载后解压出来的目录层级**不写死**，用 `_third_party_find_root()` 递归查找包根；
  - assimp 优先使用本地 `build/` 里的预编译库，找不到才从源码构建。
- `project(... LANGUAGES CXX)` → 开启 GL 后端时额外 `enable_language(C)`（glad 是 C 源码）。
- glm 以 `SYSTEM` 方式包含（`target_include_directories(... SYSTEM PUBLIC ...)`），
  避免 glm 内部的 `-Wsign-conversion` 等告警破坏 0 warning 目标。
- 新增目标 `gl_render_core_vendor`：承载 vendored 的 glad / stb，不套用本工程的严格告警集。
- `.gitignore` 放宽 `third_party/` 规则，放行 `glad/` 与 `stb/`（其余仍然忽略）。
- `.github/workflows/ci.yml`：
  - `lint` job 需要 glm，第三方目录改到 workspace 下（Linux runner 没有 D: 盘）；
  - 新增 `build-gl` job，验证 `GLRENDER_ENABLE_GL=ON` 时后端能配置并编译链接。
- 版本号由 `0.1.0` 提升到 `0.2.0`（公开 API 不兼容变更）。

### 修复

- `ShaderProgram::uniformBlockIndex` 里 `GLuint → int` 的 `-Wsign-conversion` 告警
  （此前 GL 后端从未被编译过，告警一直没暴露）。

### 验证

- MinGW + TDM-GCC 10.3.0：默认构建 **0 warning**，**31/31** 单元测试通过，示例程序正常输出。
- `GLRENDER_ENABLE_GL=ON`（`THIRD_PARTY_OFFLINE=ON`，全部命中本地库）：后端 **0 warning** 编译链接成功。

### 说明

- 取舍详见 `docs/DECISIONS.md`（D-010 取代 D-001 与 D-003）。

## [0.1.0] - 2026-10-03

### 新增

- `Vec3`：`float x, y, z`，四则运算、复合赋值、一元负号、`dot`、`cross`、`length`、`normalized`、`isZero`、`operator==`、`operator<<`；零向量归一化返回零向量。
- `Mat4`：列主序 4x4 矩阵，`identity`、`translation`、`scale`、`perspective`、`lookAt`、`operator*`、`transformPoint`（w 归一化）、`data()`、`at(row, col)`、`operator<<`。
- `util.hpp`：`kPi` / `kTwoPi` / `kHalfPi` / `kEpsilon`、`clampValue`、`degToRad`、`radToDeg`、`approxZero`、`approxEqual`。
- `ShaderSource`：`vertex` / `fragment` / `geometry`、`isComplete()`、`hasGeometry()`；自由函数 `readFile`、`fromFiles`、`fromStrings`、`stripComments`。
- `ShaderUniformCache`：`getLocation` / `setLocation` / `has` / `clear` / `size` / `names`（排序）。
- `Vertex`（`Vec3 position` + `Vec3 normal` + `float texCoordU/V`）与 `TextureInfo`（`path` / `type` / `id` / `embedded`）。
- `MeshData`：`vertexCount` / `indexCount` / `isEmpty` / `isValid` / `computeNormals`。
- `MeshDataBuilder`：从扁平 `float` 数组构造 `MeshData`，支持 3 / 5 / 6 / 8 float 每顶点的布局。
- `cubeMeshData()`：由原 `main.cpp` 的 36 顶点立方体数据构造。
- `Camera3D`：`position` / `front` / `up` / `right` / `viewMatrix` / `projectionMatrix` / `rayDirection` / `moveForward` / `moveRight` / `setYawPitch` / `update`。
- **可选 OpenGL 后端**（默认关闭，未纳入默认构建与测试）：`ShaderProgram`、`GpuMesh`、`TextureLoader`、`ModelLoader`、`Skybox`。
- 31 项单元测试、1 个示例程序。

### 变更

- `CMAKE_CXX_STANDARD` 由模板的 17 调整为 **14**。
- `Mat4::perspective` 的深度项符号与位置经测试修正（近平面映射 NDC `-1`，远平面映射 `+1`）。

### 说明

- 初始版本，由 `Ark_Like` 的渲染基础代码提炼而成。
- 与原始代码的取舍详见 `docs/DECISIONS.md`。
