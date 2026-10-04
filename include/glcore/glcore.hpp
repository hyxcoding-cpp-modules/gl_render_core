#pragma once

// ========== gl_render_core 聚合入口 ==========
// 只包含「与图形后端无关」的核心层。需要 OpenGL 后端时，
// 另外引入 glcore/gl/ 下的头文件（需开启 GLRENDER_ENABLE_GL）。
//
// 核心层的向量与矩阵直接用 glm（glm::vec3 / glm::mat4），
// 因此本头文件会把 glm 的包含目录一并带给使用者。

#include "glcore/camera3d.hpp"
#include "glcore/math.hpp"
#include "glcore/mesh_data.hpp"
#include "glcore/shader_source.hpp"
#include "glcore/util.hpp"
