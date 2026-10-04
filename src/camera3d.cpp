#include "glcore/camera3d.hpp"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "glcore/math.hpp"
#include "glcore/util.hpp"

namespace glcore
{

// C++14 下 static constexpr 成员若被 odr-use（例如取地址），需要类外定义
constexpr float Camera3D::kDefaultFovDegrees;
constexpr float Camera3D::kDefaultAspectRatio;
constexpr float Camera3D::kDefaultNearPlane;
constexpr float Camera3D::kDefaultFarPlane;
constexpr float Camera3D::kMaxPitchDegrees;

// ========== 构造 ==========
Camera3D::Camera3D(const glm::vec3& worldUp, const glm::vec3& offset)
    : m_worldUp(worldUp),
      m_origin(0.0f),
      m_offset(offset),
      m_front(0.0f, 0.0f, 1.0f),
      m_view(1.0f),
      m_projection(1.0f),
      m_fovDegrees(kDefaultFovDegrees),
      m_aspectRatio(kDefaultAspectRatio),
      m_nearPlane(kDefaultNearPlane),
      m_farPlane(kDefaultFarPlane),
      m_yawDegrees(0.0f),
      m_pitchDegrees(0.0f)
{
    update();
}

// ========== 状态设置 ==========
void Camera3D::setOrigin(const glm::vec3& origin)
{
    m_origin = origin;
}

void Camera3D::setFront(const glm::vec3& front)
{
    // 零向量会让 lookAt 退化，直接忽略
    if (isZero(front))
    {
        return;
    }
    m_front = glm::normalize(front);
}

void Camera3D::setOffset(const glm::vec3& offset)
{
    m_offset = offset;
}

void Camera3D::setFov(float fovDegrees)
{
    m_fovDegrees = fovDegrees;
}

void Camera3D::setAspectRatio(float aspectRatio)
{
    m_aspectRatio = aspectRatio;
}

void Camera3D::setNear(float nearPlane)
{
    m_nearPlane = nearPlane;
}

void Camera3D::setFar(float farPlane)
{
    m_farPlane = farPlane;
}

// ========== 查询 ==========
glm::vec3 Camera3D::position() const
{
    return m_origin + m_offset;
}

glm::vec3 Camera3D::origin() const
{
    return m_origin;
}

glm::vec3 Camera3D::offset() const
{
    return m_offset;
}

glm::vec3 Camera3D::front() const
{
    return m_front;
}

glm::vec3 Camera3D::up() const
{
    return m_worldUp;
}

glm::vec3 Camera3D::right() const
{
    // front 与 worldUp 平行时 cross 为零向量，用 normalizedOrZero 避免 NaN
    return normalizedOrZero(glm::cross(m_front, m_worldUp));
}

const glm::mat4& Camera3D::viewMatrix() const
{
    return m_view;
}

const glm::mat4& Camera3D::projectionMatrix() const
{
    return m_projection;
}

glm::vec3 Camera3D::rayDirection() const
{
    return normalizedOrZero(m_front);
}

float Camera3D::yaw() const
{
    return m_yawDegrees;
}

float Camera3D::pitch() const
{
    return m_pitchDegrees;
}

// ========== 操作 ==========
void Camera3D::moveForward(float distance)
{
    m_origin += m_front * distance;
}

void Camera3D::moveRight(float distance)
{
    m_origin += right() * distance;
}

void Camera3D::setYawPitch(float yawDegrees, float pitchDegrees)
{
    m_yawDegrees = yawDegrees;
    m_pitchDegrees = clampValue(pitchDegrees, -kMaxPitchDegrees, kMaxPitchDegrees);

    const float yawRadians = degToRad(m_yawDegrees);
    const float pitchRadians = degToRad(m_pitchDegrees);
    const float cosPitch = std::cos(pitchRadians);

    // yaw = 0、pitch = 0 时得到 (0, 0, 1)
    m_front = glm::vec3(
        std::sin(yawRadians) * cosPitch, std::sin(pitchRadians), std::cos(yawRadians) * cosPitch);
    m_front = normalizedOrZero(m_front);
}

void Camera3D::update()
{
    const glm::vec3 eye = position();
    m_view = glm::lookAt(eye, eye + m_front, m_worldUp);
    m_projection = glm::perspective(degToRad(m_fovDegrees), m_aspectRatio, m_nearPlane, m_farPlane);
}

}  // namespace glcore
