# ============================================================
# CompilerWarnings.cmake
# 按编译器设置警告级别
# 支持 MSVC、GCC、Clang
# ============================================================

# ---------- MSVC ----------
set(MSVC_WARNINGS
    /W4                 # 警告级别 4（最高常用级别）
    /permissive-        # 严格标准符合模式
    /w14242             # 格式字符串不匹配
    /w14254             # 运算符优先级
    /w14263             # 成员函数不覆盖基类
    /w14265             # 类有虚函数但无虚析构
    /w14287             # 无符号/有符号不匹配
    /w14296             # 表达式恒真/恒假
    /w14311             # 指针/整数转换
    /w14545             # 表达式前有分号
    /w14546             # 函数调用前有分号
    /w14547             # 运算符前有分号
    /w14549             # 枚举类型不匹配
    /w14555             # 表达式无效
    /w14619             # 未引用的形参
    /w14640             # 线程不安全静态初始化
    /w14826             # 有符号/无符号转换
    /w14905             # 宽字符串转窄字符串
    /w14906             # 窄字符串转宽字符串
    /w14928             # 非法拷贝初始化
    /wd4996             # 禁用“函数不安全”警告（如 strcpy）
)

# ---------- GCC / Clang 通用 ----------
set(GCC_CLANG_WARNINGS
    -Wall                       # 常用警告
    -Wextra                     # 额外警告
    -Wpedantic                  # 严格标准
    -Wshadow                    # 变量遮蔽
    -Wnon-virtual-dtor          # 有虚函数无虚析构
    -Wold-style-cast            # C 风格转换
    -Wcast-align                # 指针转换对齐
    -Wunused                    # 未使用
    -Woverloaded-virtual        # 虚函数重载
    -Wconversion                # 隐式类型转换
    -Wsign-conversion           # 有符号/无符号转换
    -Wnull-dereference          # 空指针解引用
    -Wdouble-promotion          # float 隐式转 double
    -Wformat=2                  # 格式字符串
    -Wimplicit-fallthrough      # switch 落空
)

# ---------- Clang 额外 ----------
set(CLANG_WARNINGS
    ${GCC_CLANG_WARNINGS}
    -Wno-unknown-warning-option # 忽略不认识的警告
)

# ---------- GCC 额外 ----------
set(GCC_WARNINGS
    ${GCC_CLANG_WARNINGS}
    -Wmisleading-indentation    # 缩进误导
    -Wduplicated-cond           # 重复条件
    -Wduplicated-branches       # 重复分支
    -Wlogical-op                # 逻辑运算
    -Wuseless-cast              # 无用转换
)

# ---------- 根据编译器选择 ----------
if(MSVC)
    set(PROJECT_WARNINGS ${MSVC_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(PROJECT_WARNINGS ${CLANG_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_WARNINGS ${GCC_WARNINGS})
else()
    set(PROJECT_WARNINGS "")
endif()