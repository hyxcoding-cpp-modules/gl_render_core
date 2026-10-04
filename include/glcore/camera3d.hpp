#pragma once

#include <glm/glm.hpp>

#include "glcore/math.hpp"
#include "glcore/util.hpp"

namespace glcore
{

// ========== 三维相机 ==========
// 对应原项目 camera.hpp 的 Camera：向量与矩阵直接用 glm（glm::vec3 / glm::mat4），
// 与原始实现的数学约定保持一致。
//
// 朝向约定（与原始代码一致）：
//   - yaw = 0、pitch = 0 时 front 指向 +Z，即 glm::vec3(0, 0, 1)；
//   - yaw 绕 +Y 轴旋转，pitch 为俯仰角（正值抬头）；
//   - front = (sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch))；
//   - pitch 被钳制到 [-89°, +89°]，避免 ±90° 时万向节翻转（right 向量退化）。
//
// 默认参数（未显式设置时生效）：
//   - fov      = 45 度
//   - aspect   = 1.0
//   - near     = 0.1
//   - far      = 100.0
class Camera3D
{
   public:
    static constexpr float kDefaultFovDegrees = 45.0f;
    static constexpr float kDefaultAspectRatio = 1.0f;
    static constexpr float kDefaultNearPlane = 0.1f;
    static constexpr float kDefaultFarPlane = 100.0f;
    static constexpr float kMaxPitchDegrees = 89.0f;

    // worldUp 为世界上方向（通常为 (0,1,0)）；offset 为相机相对 origin 的偏移。
    // 构造时即以默认参数计算一次 view / projection。
    explicit Camera3D(const glm::vec3& worldUp, const glm::vec3& offset = glm::vec3(0.0f));

    // ========== 状态设置 ==========
    // 各 setter 只更新状态，不自动重算矩阵；需要时调用 update()
    void setOrigin(const glm::vec3& origin);
    void setFront(const glm::vec3& front);
    void setOffset(const glm::vec3& offset);
    void setFov(float fovDegrees);
    void setAspectRatio(float aspectRatio);
    void setNear(float nearPlane);
    void setFar(float farPlane);

    // ========== 查询 ==========
    // origin + offset
    glm::vec3 position() const;
    glm::vec3 origin() const;
    glm::vec3 offset() const;

    // 已归一化的朝向向量
    glm::vec3 front() const;

    // 世界上方向（原样返回构造时传入的向量，不做归一化）
    glm::vec3 up() const;

    // 相机右方向 = normalize(cross(front, worldUp))；退化时返回零向量
    glm::vec3 right() const;

    const glm::mat4& viewMatrix() const;
    const glm::mat4& projectionMatrix() const;

    // 相机朝向射线（front 的归一化结果）
    glm::vec3 rayDirection() const;

    // ========== 操作 ==========
    // 沿 front 平移 origin
    void moveForward(float distance);

    // 沿 right 平移 origin
    void moveRight(float distance);

    // 设置 yaw / pitch（单位：度）。pitch 会被钳制到 [-89, 89]。
    void setYawPitch(float yawDegrees, float pitchDegrees);

    float yaw() const;
    float pitch() const;

    // 重算 view 与 projection 矩阵
    void update();

   private:
    glm::vec3 m_worldUp;
    glm::vec3 m_origin;
    glm::vec3 m_offset;
    glm::vec3 m_front;

    glm::mat4 m_view;
    glm::mat4 m_projection;

    float m_fovDegrees;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;

    float m_yawDegrees;
    float m_pitchDegrees;
};

}  // namespace glcore
