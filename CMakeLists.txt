cmake_minimum_required(VERSION 3.20)
project(gl_render_core VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ========== 选项 ==========
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_SHARED_LIBS "Build shared library" OFF)

# 可选后端开关：默认 OFF，不参与默认构建与测试。
# 开启后需要系统 OpenGL、glm，以及工程内 third_party/ 下的 glad / stb
# 和 glfw / assimp（本地优先，缺失时由 cmake/ThirdParty.cmake 下载）。
option(GLRENDER_ENABLE_GL "Build the optional OpenGL backend (not part of the default build)" ON)

# glad 是 C 源码，只有开启 GL 后端时才需要 C 语言
if(GLRENDER_ENABLE_GL)
    enable_language(C)
endif()

# ========== 警告 ==========
include(cmake/CompilerWarnings.cmake)

# ========== Sanitizers ==========
include(cmake/Sanitizers.cmake)

# ========== 第三方库 ==========
# 核心层的向量 / 矩阵用 glm，因此配置阶段就会定位它
# （本地写死路径优先，缺失时自动下载；详见 cmake/ThirdParty.cmake）。
include(cmake/ThirdParty.cmake)

# ========== 模块库 ==========
add_library(gl_render_core
    src/math.cpp
    src/shader_source.cpp
    src/mesh_data.cpp
    src/camera3d.cpp
)
add_library(gl_render_core::gl_render_core ALIAS gl_render_core)

target_include_directories(gl_render_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(gl_render_core PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# glm 是核心层的公开依赖（camera3d.hpp / mesh_data.hpp 直接包含 <glm/glm.hpp>），
# 必须按 SYSTEM 包含：glm 内部有 -Wsign-conversion 之类的告警，
# 用普通 -I 会污染本工程「0 warning」的目标。
# 只写 BUILD_INTERFACE：安装后由使用方自行提供 glm（见 README「依赖」一节）。
target_include_directories(gl_render_core SYSTEM PUBLIC
    $<BUILD_INTERFACE:${GLRENDER_GLM_INCLUDE_DIR}>
)

target_compile_options(gl_render_core PRIVATE ${PROJECT_WARNINGS})

# ========== 可选后端：OpenGL ==========
if(GLRENDER_ENABLE_GL)
    glrender_require_gl()

    # glad / stb 是 vendored 第三方源码，单独成一个目标：
    # 它们不适用本工程的严格告警集（glad.c 是生成代码）。
    add_library(gl_render_core_vendor
        ${GLRENDER_GLAD_SOURCE}
        ${GLRENDER_STB_SOURCE}
    )
    target_include_directories(gl_render_core_vendor SYSTEM PUBLIC
        "${GLRENDER_GLAD_DIR}/include"
        "${GLRENDER_STB_INCLUDE_DIR}"
    )

    add_library(gl_render_core_gl
        src/gl/shader_program.cpp
        src/gl/gpu_mesh.cpp
        src/gl/texture_loader.cpp
        src/gl/model_loader.cpp
        src/gl/skybox.cpp
    )
    add_library(gl_render_core::gl ALIAS gl_render_core_gl)

    target_link_libraries(gl_render_core_gl
        PUBLIC gl_render_core::gl_render_core
        PRIVATE gl_render_core_vendor ${GLRENDER_GL_LIBRARIES}
    )
    # 第三方头文件（glad / stb / assimp）一律按系统头处理，避免它们的告警进入本工程
    target_include_directories(gl_render_core_gl SYSTEM PRIVATE ${GLRENDER_GL_INCLUDE_DIRS})
    target_compile_options(gl_render_core_gl PRIVATE ${PROJECT_WARNINGS})

    # Windows 下 assimp 是动态库，把 dll 拷到产物目录，方便直接运行
    if(WIN32 AND GLRENDER_ASSIMP_RUNTIME_DLLS)
        add_custom_command(TARGET gl_render_core_gl POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${GLRENDER_ASSIMP_RUNTIME_DLLS}
                    $<TARGET_FILE_DIR:gl_render_core_gl>
            COMMENT "拷贝 assimp 运行期 DLL")
    endif()
endif()

# ========== 测试 ==========
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ========== 示例 ==========
if(BUILD_EXAMPLES)
    add_executable(example_gl_render_core examples/example_gl_render_core.cpp)
    target_link_libraries(example_gl_render_core PRIVATE gl_render_core::gl_render_core)
endif()

# ========== 安装 ==========
include(GNUInstallDirs)

install(TARGETS gl_render_core
    EXPORT gl_render_core-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT gl_render_core-targets
    FILE gl_render_core-targets.cmake
    NAMESPACE gl_render_core::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gl_render_core
)
