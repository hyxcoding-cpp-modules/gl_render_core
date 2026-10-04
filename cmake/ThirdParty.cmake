# ============================================================
# ThirdParty.cmake
# 第三方库统一入口：**本地路径写死 + 缺失自动下载解压 + 下载后动态定位**
#
# 设计目标
# --------
# 1. 所有第三方库统一放在 THIRD_PARTY_DIR，默认**写死**为 D:/third_party。
# 2. 每个库在本地的位置也在下面**直接写死**（见各节的 XXX_DIR），
#    不做 find_package / find_path 之类的全盘搜索。
# 3. 配置时先看写死的路径：
#      命中   → 直接使用本地副本，不联网；
#      落空   → 自动下载对应的官方压缩包到 THIRD_PARTY_DIR，解压后**删除压缩包**。
# 4. 压缩包解压出来的顶层目录名由上游决定，**代码里不写死层级**：
#    下载完成后用 _third_party_find_root() 递归查找「包含标志文件的目录」，
#    找到的那一层就是包根。上游改了目录结构也不会让 CI 挂掉。
# 5. 不使用 git clone。
#
# 本地目录布局（与官方压缩包解压后的形态一致）
# --------------------------------------------
#   D:/third_party/
#     glm/                      glm 源码（只用到 glm/glm.hpp 等头文件）
#     googletest-1.18.0/        googletest 源码，本文件会在本构建树里编译它
#     OpenGL/glfw-3.4/          glfw 源码，本文件会在本构建树里编译它
#     assimp-5.4.3/             assimp 源码；build/ 里已带预编译 lib + dll
#
# 提供的能力
# ----------
#   third_party_download(<url> <压缩包文件名> <解压目标目录>)
#   third_party_ensure(<OUT_VAR> <name> DIR <路径> MARKER <标志文件>...
#                      [LIBS <库文件>...] [URL <下载地址>] [DEST <解压目录>] [DEPTH <层数>])
#   third_party_add(<name> <目录>)
#   third_party_require_googletest()
#   glrender_require_glm()
#   glrender_require_glfw()
#   glrender_require_assimp()
#   glrender_require_gl()
#
# 可调选项
# --------
#   THIRD_PARTY_DIR          第三方根目录，默认写死 D:/third_party
#                            （CI 等异构环境可用 -DTHIRD_PARTY_DIR=... 覆盖）
#   THIRD_PARTY_OFFLINE      ON 时禁止联网下载，本地找不到就直接报错
#   THIRD_PARTY_TLS_VERIFY   下载时校验 TLS 证书，默认 ON。
#                            若本机证书吊销检查异常导致 'SSL connect error'，
#                            可加 -DTHIRD_PARTY_TLS_VERIFY=OFF 重试。
#   THIRD_PARTY_SEARCH_DEPTH 下载后递归查找包根的最大层数，默认 4
# ============================================================

set(THIRD_PARTY_DIR "D:/third_party"
    CACHE PATH "第三方依赖根目录（默认写死为 D:/third_party）")
option(THIRD_PARTY_OFFLINE "禁止第三方依赖联网下载，只使用本地已有副本" OFF)
option(THIRD_PARTY_TLS_VERIFY "下载第三方包时校验 TLS 证书" ON)
set(THIRD_PARTY_SEARCH_DEPTH "4"
    CACHE STRING "下载后递归查找包根目录的最大层数")

# 本仓库根目录（本文件位于 <root>/cmake/ 下）
get_filename_component(GLRENDER_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# ============================================================
# third_party_download(<url> <压缩包文件名> <解压目标目录>)
#
# 下载 → 解压 → 删除压缩包。任一步失败都给出中文提示并清理残留压缩包。
# ============================================================
function(third_party_download URL ARCHIVE_NAME DEST_DIR)
    if(THIRD_PARTY_OFFLINE)
        message(FATAL_ERROR
            "[third_party] 需要下载 ${URL}，但 THIRD_PARTY_OFFLINE=ON。\n"
            "  请手动把开发包解压到 ${DEST_DIR}，或去掉该选项。")
    endif()

    file(MAKE_DIRECTORY "${DEST_DIR}")
    set(_archive "${DEST_DIR}/${ARCHIVE_NAME}")

    message(STATUS "[third_party] 正在下载 ${URL}")
    # 注意：CMake 4 的 file(DOWNLOAD) 已经默认跟随重定向（github.com →
    # objects.githubusercontent.com），不再有 FOLLOW_REDIRECTS 参数，
    # 写了会被当作未知参数并产生 author warning。
    file(DOWNLOAD "${URL}" "${_archive}"
        STATUS             _tp_dl_status
        LOG                _tp_dl_log
        TLS_VERIFY         ${THIRD_PARTY_TLS_VERIFY}
        INACTIVITY_TIMEOUT 60
        TIMEOUT            900
        SHOW_PROGRESS)

    list(GET _tp_dl_status 0 _tp_dl_code)
    if(NOT _tp_dl_code EQUAL 0)
        list(GET _tp_dl_status 1 _tp_dl_msg)
        file(REMOVE "${_archive}")
        message(FATAL_ERROR
            "[third_party] 下载失败：${URL}\n"
            "  原因：${_tp_dl_msg}\n"
            "  详情：${_tp_dl_log}\n"
            "  若原因是 'SSL connect error' 之类的证书校验失败，"
            "可用 -DTHIRD_PARTY_TLS_VERIFY=OFF 重试；\n"
            "  也可以手动把开发包解压到 ${DEST_DIR}。")
    endif()

    message(STATUS "[third_party] 正在解压 ${ARCHIVE_NAME}")
    file(ARCHIVE_EXTRACT INPUT "${_archive}" DESTINATION "${DEST_DIR}")

    file(REMOVE "${_archive}")
    message(STATUS "[third_party] 解压完成，已删除压缩包 ${_archive}")
endfunction()

# ============================================================
# _third_party_print_tree(<目录> <最多列出的文件数> <OUT_VAR>)
#
# 把目录树拼成一段文本，用于下载后定位失败时的报错，方便一眼看出
# 上游把压缩包解成了什么结构。
# ============================================================
function(_third_party_print_tree DIR LIMIT OUT_VAR)
    set(_lines "")
    if(IS_DIRECTORY "${DIR}")
        file(GLOB_RECURSE _files LIST_DIRECTORIES false "${DIR}/*")
        list(LENGTH _files _total)
        set(_shown 0)
        foreach(_file IN LISTS _files)
            if(_shown LESS LIMIT)
                file(RELATIVE_PATH _rel "${DIR}" "${_file}")
                string(APPEND _lines "    ${_rel}\n")
                math(EXPR _shown "${_shown} + 1")
            endif()
        endforeach()
        if(_total GREATER LIMIT)
            math(EXPR _rest "${_total} - ${LIMIT}")
            string(APPEND _lines "    ...（另有 ${_rest} 个文件未列出）\n")
        endif()
    else()
        set(_lines "    （目录不存在）\n")
    endif()
    set(${OUT_VAR} "${_lines}" PARENT_SCOPE)
endfunction()

# ============================================================
# _third_party_find_root(<OUT_VAR> <起始目录> <标志文件列表> <库文件列表> <最大层数>)
#
# 从 DIR 自己开始，逐层向下查找「包根目录」。
# 命中条件：MARKERS 里的标志文件**全部存在**，且（LIBS 为空 或 LIBS 里至少有一个存在）。
#
# 两个要点：
#   - 先 list(SORT) 再递归，保证同一份目录结构下每次得到的结果一致；
#   - 参数可能为空（例如没有 LIBS），调用方必须用引号传参（"${LIBS}"），
#     否则 CMake 会把空串吃掉、导致后续参数整体错位。
# ============================================================
function(_third_party_find_root OUT_VAR DIR MARKERS LIBS DEPTH)
    set(_found "")
    set(_level_dirs "${DIR}")
    set(_level 0)

    while(NOT _found AND _level_dirs AND _level LESS DEPTH)
        set(_next_dirs "")
        foreach(_dir IN LISTS _level_dirs)
            if(NOT IS_DIRECTORY "${_dir}")
                continue()
            endif()

            set(_match TRUE)
            foreach(_marker IN LISTS MARKERS)
                if(NOT EXISTS "${_dir}/${_marker}")
                    set(_match FALSE)
                endif()
            endforeach()

            if(_match AND NOT "${LIBS}" STREQUAL "")
                set(_lib_match FALSE)
                foreach(_lib IN LISTS LIBS)
                    if(EXISTS "${_dir}/${_lib}")
                        set(_lib_match TRUE)
                    endif()
                endforeach()
                if(NOT _lib_match)
                    set(_match FALSE)
                endif()
            endif()

            if(_match)
                set(_found "${_dir}")
                break()
            endif()

            # LIST_DIRECTORIES true 会把文件也列出来，必须自己过滤掉
            file(GLOB _children LIST_DIRECTORIES true "${_dir}/*")
            foreach(_child IN LISTS _children)
                if(IS_DIRECTORY "${_child}")
                    list(APPEND _next_dirs "${_child}")
                endif()
            endforeach()
        endforeach()

        if(NOT _found)
            list(SORT _next_dirs)
            set(_level_dirs "${_next_dirs}")
        endif()
        math(EXPR _level "${_level} + 1")
    endwhile()

    set(${OUT_VAR} "${_found}" PARENT_SCOPE)
endfunction()

# ============================================================
# third_party_ensure(<OUT_VAR> <name>
#     DIR    <该库在本地的绝对路径>        # 写死的路径
#     MARKER <判定「已就绪」的标志文件>    # 相对包根；可重复给多个
#     [LIBS  <必须存在的库文件>...]        # 可重复；用来排除「同名但缺少库」的目录
#     [URL   <官方压缩包下载地址>]
#     [DEST  <解压目标目录>]               # 默认 THIRD_PARTY_DIR
#     [DEPTH <递归查找最大层数>]           # 默认 THIRD_PARTY_SEARCH_DEPTH
# )
#
# 本地已有 → 直接用；本地没有 → 下载并解压，然后**动态定位**包根。
# 最终把「包根目录的绝对路径」写进 OUT_VAR（PARENT_SCOPE）。
# 只负责「把文件弄到位」，需要从源码编译的库请接着调用 third_party_add()。
# ============================================================
function(third_party_ensure OUT_VAR NAME)
    cmake_parse_arguments(TP "" "DIR;URL;DEST;DEPTH" "MARKER;LIBS" ${ARGN})

    if(NOT TP_DIR)
        message(FATAL_ERROR "[third_party] ${NAME}: 调用 third_party_ensure() 时必须提供 DIR")
    endif()
    if(NOT TP_MARKER)
        set(TP_MARKER "CMakeLists.txt")
    endif()
    if(NOT TP_DEPTH)
        set(TP_DEPTH "${THIRD_PARTY_SEARCH_DEPTH}")
    endif()

    # ---------- 1. 本地优先 ----------
    _third_party_find_root(_root "${TP_DIR}" "${TP_MARKER}" "${TP_LIBS}" "${TP_DEPTH}")
    if(_root)
        message(STATUS "[third_party] ${NAME}: 本地命中 ${_root}")
        set(${OUT_VAR} "${_root}" PARENT_SCOPE)
        return()
    endif()

    # ---------- 2. 本地没有 → 下载 ----------
    if(NOT TP_URL)
        message(FATAL_ERROR
            "[third_party] ${NAME}: 本地未找到 ${TP_DIR}（标志：${TP_MARKER}），且未提供下载地址。\n"
            "  请手动把对应的开发包解压到 ${TP_DIR} 下。")
    endif()

    set(_dest "${TP_DEST}")
    if(NOT _dest)
        set(_dest "${THIRD_PARTY_DIR}")
    endif()

    get_filename_component(_archive_name "${TP_URL}" NAME)
    third_party_download("${TP_URL}" "${_archive_name}" "${_dest}")

    # ---------- 3. 下载后动态定位（不假设压缩包解出来的层级） ----------
    _third_party_find_root(_root "${TP_DIR}" "${TP_MARKER}" "${TP_LIBS}" "${TP_DEPTH}")
    if(NOT _root)
        _third_party_find_root(_root "${_dest}" "${TP_MARKER}" "${TP_LIBS}" "${TP_DEPTH}")
    endif()

    if(NOT _root)
        _third_party_print_tree("${_dest}" 40 _tree)
        message(FATAL_ERROR
            "[third_party] ${NAME}: 下载并解压后仍未找到标志文件。\n"
            "  压缩包：${TP_URL}\n"
            "  解压到：${_dest}\n"
            "  查找标志：${TP_MARKER}\n"
            "  ${_dest} 下的内容：\n${_tree}")
    endif()

    message(STATUS "[third_party] ${NAME}: 下载解压后定位到 ${_root}")
    set(${OUT_VAR} "${_root}" PARENT_SCOPE)
endfunction()

# ============================================================
# third_party_add(<name> <目录>)
# 把目录加入构建树并编译（用于需要从源码构建的库）。
# 用 macro 而非 function，避免 add_subdirectory 的作用域嵌套带来的变量可见性问题。
# ============================================================
macro(third_party_add NAME DIR)
    add_subdirectory("${DIR}" "${CMAKE_BINARY_DIR}/third_party/${NAME}")
endmacro()

# ============================================================
# 具体依赖
# ============================================================

# ------------------------------------------------------------
# Google Test（单元测试框架，所有模块的测试都依赖它）
# ------------------------------------------------------------
# googletest 官方只提供源码包，因此这里下载官方源码后在本构建树里编译。
# 版本与原先 vendored 的副本保持一致（1.18.0），避免行为漂移。
set(GOOGLETEST_DIR "${THIRD_PARTY_DIR}/googletest-1.18.0")
set(GOOGLETEST_URL "https://github.com/google/googletest/archive/refs/tags/v1.18.0.zip")

function(third_party_require_googletest)
    if(TARGET GTest::gtest_main)
        return()
    endif()

    third_party_ensure(GOOGLETEST_ROOT googletest
        DIR    "${GOOGLETEST_DIR}"
        MARKER "googletest/include/gtest/gtest.h"
        LIBS   "CMakeLists.txt"
        URL    "${GOOGLETEST_URL}"
        DEST   "${THIRD_PARTY_DIR}")

    set(gtest_force_shared_crt ON CACHE BOOL "让 gtest 使用与上层工程一致的 CRT" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "不把 gtest 装进本模块的 install 集合" FORCE)
    set(BUILD_GMOCK ON CACHE BOOL "编译 gmock（gtest_main 之外常需）" FORCE)

    third_party_add(googletest "${GOOGLETEST_ROOT}")
endfunction()

# ------------------------------------------------------------
# glm（向量 / 矩阵数学库，仅头文件）
# ------------------------------------------------------------
# 核心层的公开依赖：include/glcore/camera3d.hpp、mesh_data.hpp 等直接包含 <glm/glm.hpp>。
# 因此只要有构建就必须先把它定位好，本文件在 include 时就会调用一次。
set(GLM_DIR "${THIRD_PARTY_DIR}/glm")
set(GLM_URL "https://github.com/g-truc/glm/releases/download/1.0.3/glm-1.0.3.zip")

function(glrender_require_glm)
    if(GLRENDER_GLM_INCLUDE_DIR)
        return()
    endif()

    third_party_ensure(GLM_ROOT glm
        DIR    "${GLM_DIR}"
        MARKER "glm/glm.hpp"
        URL    "${GLM_URL}"
        DEST   "${THIRD_PARTY_DIR}")

    # 用 CACHE INTERNAL 保存，保证在任何作用域里都能读到
    set(GLRENDER_GLM_INCLUDE_DIR "${GLM_ROOT}"
        CACHE INTERNAL "glm 头文件根目录（由 cmake/ThirdParty.cmake 定位）" FORCE)
    message(STATUS "[third_party] glm: 头文件目录 ${GLRENDER_GLM_INCLUDE_DIR}")
endfunction()

# 核心层依赖 glm，include 本文件时即完成定位
glrender_require_glm()

# ------------------------------------------------------------
# 可选 OpenGL 后端的依赖
# ------------------------------------------------------------
# glad / stb：直接放在本仓库 third_party/ 下（源码），随仓库提交，不参与下载。
set(GLRENDER_GLAD_DIR "${GLRENDER_PROJECT_ROOT}/third_party/glad")
set(GLRENDER_STB_DIR "${GLRENDER_PROJECT_ROOT}/third_party/stb")
# 代码里写的是 #include <stb/stb_image.h>，所以包含目录要指向 stb 的上一级
set(GLRENDER_STB_INCLUDE_DIR "${GLRENDER_PROJECT_ROOT}/third_party")

# glfw：本地优先；缺失时下载官方源码包，然后调用它自己的 CMakeLists 编译。
set(GLRENDER_GLFW_DIR "${THIRD_PARTY_DIR}/OpenGL/glfw-3.4")
set(GLRENDER_GLFW_URL "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip")

# assimp：本地优先（D:/third_party/assimp-5.4.3/build 里已有预编译 lib + dll）；
# 缺失时下载官方源码包，然后调用它自己的 CMakeLists 编译。
set(GLRENDER_ASSIMP_DIR "${THIRD_PARTY_DIR}/assimp-5.4.3")
set(GLRENDER_ASSIMP_URL "https://github.com/assimp/assimp/archive/refs/tags/v5.4.3.zip")

function(glrender_require_glfw)
    if(TARGET glfw)
        return()
    endif()

    third_party_ensure(GLRENDER_GLFW_ROOT glfw
        DIR    "${GLRENDER_GLFW_DIR}"
        MARKER "CMakeLists.txt"
        MARKER "include/GLFW/glfw3.h"
        URL    "${GLRENDER_GLFW_URL}"
        DEST   "${THIRD_PARTY_DIR}")

    # 只要库本身：示例 / 测试 / 文档 / 安装一律不做
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "不构建 glfw 示例" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "不构建 glfw 测试" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "不构建 glfw 文档" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "不把 glfw 装进本模块的 install 集合" FORCE)

    third_party_add(glfw "${GLRENDER_GLFW_ROOT}")
endfunction()

function(glrender_require_assimp)
    if(TARGET assimp::assimp OR TARGET assimp)
        return()
    endif()

    third_party_ensure(GLRENDER_ASSIMP_ROOT assimp
        DIR    "${GLRENDER_ASSIMP_DIR}"
        MARKER "include/assimp/scene.h"
        URL    "${GLRENDER_ASSIMP_URL}"
        DEST   "${THIRD_PARTY_DIR}")

    # ---------- 预编译优先：本地包里的 build/ 已经带了 lib + dll ----------
    set(_prebuilt_lib "")
    foreach(_rel
            "build/lib/libassimp.dll.a"
            "build/lib/libassimp.a"
            "build/lib/assimp.lib"
            "lib/libassimp.dll.a"
            "lib/libassimp.a"
            "lib/assimp.lib"
            "lib64/libassimp.so"
            "lib/libassimp.so")
        if(EXISTS "${GLRENDER_ASSIMP_ROOT}/${_rel}")
            set(_prebuilt_lib "${GLRENDER_ASSIMP_ROOT}/${_rel}")
            break()
        endif()
    endforeach()

    if(_prebuilt_lib)
        message(STATUS "[third_party] assimp: 使用预编译库 ${_prebuilt_lib}")
        set(GLRENDER_ASSIMP_LIBRARIES "${_prebuilt_lib}")
        set(GLRENDER_ASSIMP_INCLUDE_DIRS
            "${GLRENDER_ASSIMP_ROOT}/build/include"
            "${GLRENDER_ASSIMP_ROOT}/include")
        file(GLOB GLRENDER_ASSIMP_DLLS
             "${GLRENDER_ASSIMP_ROOT}/build/bin/*.dll"
             "${GLRENDER_ASSIMP_ROOT}/bin/*.dll")
    else()
        message(STATUS "[third_party] assimp: 未找到预编译库，改为从源码构建")
        # assimp 自带 CMakeLists：它会把自己的 CMAKE_CXX_STANDARD 设为 17，
        # 由于 add_subdirectory 会新建作用域，这个改动只作用于它自己的子树。
        set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "不构建 assimp 测试（依赖 contrib/googletest）" FORCE)
        set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "不构建 assimp 命令行工具" FORCE)
        set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "不构建 assimp 示例" FORCE)
        set(ASSIMP_INSTALL OFF CACHE BOOL "不把 assimp 装进本模块的 install 集合" FORCE)
        set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "不把 assimp 自身的告警当错误" FORCE)
        set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "用 assimp 自带的 zlib，避免额外依赖" FORCE)
        set(ASSIMP_NO_EXPORT ON CACHE BOOL "关闭 assimp 的导出配置" FORCE)
        third_party_add(assimp "${GLRENDER_ASSIMP_ROOT}")

        set(GLRENDER_ASSIMP_LIBRARIES assimp)
        set(GLRENDER_ASSIMP_INCLUDE_DIRS
            "${GLRENDER_ASSIMP_ROOT}/include"
            "${CMAKE_BINARY_DIR}/third_party/assimp/include")
        set(GLRENDER_ASSIMP_DLLS "")
    endif()

    set(GLRENDER_ASSIMP_INCLUDE_DIRS "${GLRENDER_ASSIMP_INCLUDE_DIRS}" PARENT_SCOPE)
    set(GLRENDER_ASSIMP_LIBRARIES "${GLRENDER_ASSIMP_LIBRARIES}" PARENT_SCOPE)
    set(GLRENDER_ASSIMP_DLLS "${GLRENDER_ASSIMP_DLLS}" PARENT_SCOPE)
endfunction()

# ============================================================
# glrender_require_gl()
# 一次性把可选 OpenGL 后端要用的东西全部就位，并汇总出顶层 CMakeLists 需要的变量：
#   GLRENDER_GLAD_SOURCE           glad 源码（C）
#   GLRENDER_STB_SOURCE            stb_image 的实现包装（C++）
#   GLRENDER_GL_INCLUDE_DIRS       第三方头文件目录
#   GLRENDER_GL_LIBRARIES          需要链接的目标 / 库
#   GLRENDER_ASSIMP_RUNTIME_DLLS   Windows 下需要随产物拷贝的 dll
# ============================================================
function(glrender_require_gl)
    if(GLRENDER_GL_READY)
        return()
    endif()

    # ---------- 1. 系统 OpenGL ----------
    find_package(OpenGL REQUIRED)

    # ---------- 2. glm（核心层已依赖，这里只保证已定位） ----------
    glrender_require_glm()

    # ---------- 3. glad / stb：工程内 vendored 源码，不参与下载 ----------
    if(NOT EXISTS "${GLRENDER_GLAD_DIR}/include/glad/glad.h"
       OR NOT EXISTS "${GLRENDER_GLAD_DIR}/src/glad.c")
        message(FATAL_ERROR
            "[third_party] glad: 未找到 ${GLRENDER_GLAD_DIR}/include/glad/glad.h 或 src/glad.c。\n"
            "  glad 是本仓库 third_party/ 下的 vendored 源码，不参与下载，"
            "请确认它已随仓库一起提交。")
    endif()
    if(NOT EXISTS "${GLRENDER_STB_DIR}/stb_image.h"
       OR NOT EXISTS "${GLRENDER_STB_DIR}/stb_image_wrapper.cpp")
        message(FATAL_ERROR
            "[third_party] stb: 未找到 ${GLRENDER_STB_DIR}/stb_image.h 或 "
            "stb_image_wrapper.cpp。\n"
            "  stb 是本仓库 third_party/ 下的 vendored 源码，不参与下载，"
            "请确认它已随仓库一起提交。")
    endif()

    # ---------- 4. glfw / assimp：本地优先，缺失下载源码包后编译 ----------
    glrender_require_glfw()
    glrender_require_assimp()

    # ---------- 5. 汇总 ----------
    set(GLRENDER_GLAD_SOURCE "${GLRENDER_GLAD_DIR}/src/glad.c" PARENT_SCOPE)
    set(GLRENDER_STB_SOURCE "${GLRENDER_STB_DIR}/stb_image_wrapper.cpp" PARENT_SCOPE)
    set(GLRENDER_GL_INCLUDE_DIRS
        "${GLRENDER_GLAD_DIR}/include"
        "${GLRENDER_STB_INCLUDE_DIR}"
        ${GLRENDER_ASSIMP_INCLUDE_DIRS}
        PARENT_SCOPE)
    set(GLRENDER_GL_LIBRARIES
        glfw
        ${GLRENDER_ASSIMP_LIBRARIES}
        OpenGL::GL
        PARENT_SCOPE)
    set(GLRENDER_ASSIMP_RUNTIME_DLLS "${GLRENDER_ASSIMP_DLLS}" PARENT_SCOPE)
    set(GLRENDER_GL_READY ON PARENT_SCOPE)
endfunction()
