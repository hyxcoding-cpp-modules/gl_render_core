#pragma once

#include <glm/glm.hpp>
#include <string>

#include "glcore/util.hpp"

namespace glcore
{

// ========== glm 补充工具 ==========
// 本模块的向量与矩阵运算全部交给 glm（<glm/glm.hpp>、<glm/gtc/matrix_transform.hpp> 等），
// 这里**不实现任何数学逻辑**，只补上 glm 没有提供、而本模块反复需要的一点点便利：
//
//   isZero()             glm 没有「近似为零」判定，而零向量会让 normalize 产生 NaN；
//   normalizedOrZero()   对零向量返回零向量，而不是 glm::normalize 的 NaN；
//   toString()           便于日志与示例输出（glm 未提供 operator<<）。

// ========== 零向量判定 ==========
// 三个分量都近似为零
inline bool isZero(const glm::vec3& value)
{
    return approxZero(static_cast<double>(value.x)) && approxZero(static_cast<double>(value.y))
           && approxZero(static_cast<double>(value.z));
}

// ========== 零向量安全的归一化 ==========
// 长度为 0 时返回零向量，不产生 NaN
inline glm::vec3 normalizedOrZero(const glm::vec3& value)
{
    const float len = glm::length(value);
    if (approxZero(static_cast<double>(len)))
    {
        return glm::vec3(0.0f);
    }
    return value / len;
}

// ========== 文本输出 ==========
// 形如 "(1,2,3)"
std::string toString(const glm::vec3& value);

// 四行，每行形如 "[ 1 , 0 , 0 , 0 ]"
std::string toString(const glm::mat4& value);

}  // namespace glcore
