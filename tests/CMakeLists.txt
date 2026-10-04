# ========== Google Test ==========
# 使用 D:/third_party/googletest-1.18.0；不存在时由 cmake/ThirdParty.cmake 自动下载并构建。
third_party_require_googletest()

# ========== 测试可执行文件 ==========
add_executable(test_gl_render_core test_gl_render_core.cpp)
target_link_libraries(test_gl_render_core PRIVATE gl_render_core::gl_render_core GTest::gtest_main)
target_compile_options(test_gl_render_core PRIVATE ${PROJECT_WARNINGS})

# ========== 注册测试 ==========
include(GoogleTest)
gtest_discover_tests(test_gl_render_core)
