# third_party

本目录只放**两类**东西：

1. **vendored 源码**——`glad/` 与 `stb/`，随仓库一起提交，CI 上 `checkout` 就能拿到；
2. 这份说明文档。

其余第三方依赖统一放在 `D:/third_party`，由 `cmake/ThirdParty.cmake` 在配置阶段检查、缺失时下载。

## 为什么这样做

原先 googletest 的完整源码（约 250 个文件）被提交进版本库，带来几个问题：

- 仓库体积大、克隆慢；
- vendored 代码混进自己的 diff / 统计 / 换行符与格式规则里，需要额外排除；
- 想升级测试框架要手工替换一大堆文件。

改成「路径写死 + 缺失自动下载」之后，仓库里只剩自己的代码，
第三方依赖集中在一处（`D:/third_party`），所有模块共用同一份副本。

`glad` 与 `stb` 是例外：它们不是通过包管理器分发的完整项目，而是少量源码文件，
且**必须和本模块的代码一起版本化**（CI 上没有它们就没法编译 GL 后端），所以留在本目录里。
它们的包含关系是 `#include <glad/glad.h>`、`#include <stb/stb_image.h>`，
因此 glad 的包含目录是 `third_party/glad/include`，stb 的是 `third_party/`（stb 的上一级）。

## 工作方式

`cmake/ThirdParty.cmake` 提供这些能力：

| 函数 | 作用 |
|------|------|
| `third_party_download(<url> <压缩包名> <解压目录>)` | 下载 → 解压 → **删除压缩包** |
| `third_party_ensure(<OUT_VAR> <name> DIR <路径> MARKER <标志文件>... [LIBS <库文件>...] [URL <地址>] [DEST <目录>] [DEPTH <层数>])` | 写死的路径命中 → 直接用；落空 → 下载解压，然后**动态定位包根**并把结果写进 `OUT_VAR` |
| `third_party_add(<name> <目录>)` | 把目录加入构建树（`add_subdirectory`），即**到位后立即编译** |
| `third_party_require_googletest()` | 上面两步的组合，供测试使用 |
| `glrender_require_glm()` / `glrender_require_glfw()` / `glrender_require_assimp()` / `glrender_require_gl()` | 可选 OpenGL 后端的依赖 |

判定「本地已有」靠 `MARKER`：必须**全部存在**才算命中；再给 `LIBS` 时，
还要求 `LIBS` 里至少有一个存在（用来排除「目录名对得上、但里面是另一套编译器的包」）。

### 下载后不写死目录层级

压缩包解压出来的顶层目录名由上游决定，代码里**不做任何假设**。
`_third_party_find_root(OUT_VAR DIR MARKERS LIBS DEPTH)` 会：

1. 先看 `DIR` 自己；
2. 不命中就 `file(GLOB)` 出子目录、`list(SORT)` 排序后逐层向下，最多 `DEPTH` 层；
3. 返回第一个「标志文件齐全」的目录。

排序保证了同一份目录结构下每次得到的结果一致。查找失败时会用
`_third_party_print_tree()` 把目录树打印出来，一眼就能看出上游把包解成了什么结构。

调用位置：核心层需要 glm，所以 `cmake/ThirdParty.cmake` 在 `include()` 时就会定位 glm；
googletest 放在 `tests/CMakeLists.txt`：

```cmake
third_party_require_googletest()
target_link_libraries(test_xxx PRIVATE xxx::xxx GTest::gtest_main)
```

放在 `tests/` 而不是顶层，是为了让 `-DBUILD_TESTS=OFF` 时不白白下载和编译 gtest。

## 当前依赖

| 名称 | 本地路径（写死） | 版本 | 缺失时的下载地址 | 用途 |
|------|------------------|------|------------------|------|
| glm | `D:/third_party/glm` | 1.0.3 | `glm-1.0.3.zip` | 向量 / 矩阵（**核心层公开依赖**） |
| googletest | `D:/third_party/googletest-1.18.0` | 1.18.0 | `v1.18.0.zip` | 单元测试框架 |
| glfw | `D:/third_party/OpenGL/glfw-3.4` | 3.4 | `glfw-3.4.zip`（源码） | 窗口 / GL 上下文 |
| assimp | `D:/third_party/assimp-5.4.3` | 5.4.3 | `v5.4.3.zip`（源码） | 模型加载 |

glfw 与 assimp 下载的都是**源码包**，命中后调用它们自己的 `CMakeLists.txt` 编译，
因此与编译器类型、平台类型无关。assimp 若在本地 `build/lib` 下已有预编译库，
则直接链接、不重复编译源码。

要升级只需改 `cmake/ThirdParty.cmake` 里对应的 `XXX_DIR` 与 `XXX_URL`。

## 目录结构

配置一次之后 `D:/third_party` 会变成：

```
D:/third_party/
  glm/                    glm 源码（缺失时由本模块自动下载）
  googletest-1.18.0/      googletest 源码
  OpenGL/glfw-3.4/        glfw 源码
  assimp-5.4.3/           assimp 源码（build/ 里可能已带预编译 lib + dll）
```

下载解压出来的目录名可能带版本号（例如 `glm-1.0.3`），这不影响使用——
定位靠的是递归查找标志文件，不是目录名。

本模块目录里则是：

```
third_party/
  README.md              提交进版本库
  glad/                  vendored 源码，提交进版本库
  stb/                   vendored 源码，提交进版本库
```

## 离线 / 内网环境

两种办法：

**1) 手动放置文件**（推荐）

把官方压缩包解压到写死的路径（例如 `D:/third_party/glm`，保证里面有 `glm/glm.hpp`），
`third_party_ensure` 会直接使用它，不联网。放到写死路径**下面任意层级**也能被找到。

**2) 打开离线开关**

```bash
cmake -B build/debug -DTHIRD_PARTY_OFFLINE=ON
```

此时本地找不到就直接报错并提示路径，不会尝试联网 —— 适合确定没有外网、
希望尽早暴露问题的环境。

## 下载失败怎么办

配置阶段报 `[third_party] 下载失败` 时，错误信息里会带上 CMake 的下载详情与状态码。
常见两种情况：

- **`SSL connect error` / 证书校验失败**

  本机的证书吊销检查可能异常（Windows 上 git 也常需 `GIT_SSL_NO_VERIFY=1`）。
  临时办法是关掉下载时的 TLS 校验：

  ```bash
  cmake -B build/debug -DTHIRD_PARTY_TLS_VERIFY=OFF
  ```

  也可以用环境变量 `CMAKE_TLS_VERIFY=0`。根治办法是修好系统根证书 /
  关掉「检查证书吊销」策略。

- **下载太慢或完全没有外网**

  直接把官方压缩包解压到写死的路径（见上），或用 `-DTHIRD_PARTY_OFFLINE=ON`
  让问题尽早暴露。

## 路径与 CI

- `THIRD_PARTY_DIR` 默认**写死**为 `D:/third_party`，不做任何搜索。
  异构环境（例如 Linux runner 没有 D: 盘）可以用 `-DTHIRD_PARTY_DIR=...` 覆盖，
  下载出来的内容会落在覆盖后的目录里 —— CI 的 `lint` job 就是这么做的。
- `.github/workflows/ci.yml` 用 `actions/cache` 缓存第三方目录，
  缓存 key 基于 `cmake/ThirdParty.cmake` 的哈希：

  - 第一次跑：下载一次；
  - 后续跑：命中缓存，跳过下载；
  - 改了依赖版本（`XXX_URL`）→ key 变化 → 自动重新下载。

  所以升级依赖不需要手工清理任何缓存。
- `THIRD_PARTY_SEARCH_DEPTH`（默认 4）控制下载后递归查找的最大层数，
  上游把包套得特别深时可以调大。
