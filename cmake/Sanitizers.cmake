# ============================================================
# Sanitizers.cmake
# 支持 AddressSanitizer、UndefinedBehaviorSanitizer
# 支持 MSVC、GCC、Clang
# ============================================================

# ---------- AddressSanitizer ----------
if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled")

    if(MSVC)
        # MSVC: /fsanitize=address
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    else()
        # GCC / Clang: -fsanitize=address
        add_compile_options(
            -fsanitize=address
            -fno-omit-frame-pointer
            -g
        )
        add_link_options(-fsanitize=address)
    endif()
endif()

# ---------- UndefinedBehaviorSanitizer ----------
if(ENABLE_UBSAN)
    if(MSVC)
        # MSVC 不支持 UBSan
        message(WARNING "UBSan is not supported on MSVC, ignoring ENABLE_UBSAN")
    else()
        message(STATUS "UndefinedBehaviorSanitizer enabled")
        add_compile_options(
            -fsanitize=undefined
            -fno-omit-frame-pointer
            -g
        )
        add_link_options(-fsanitize=undefined)
    endif()
endif()

# ---------- 组合检查 ----------
if(ENABLE_ASAN AND ENABLE_UBSAN AND NOT MSVC)
    message(STATUS "Both ASan and UBSan enabled")
endif()