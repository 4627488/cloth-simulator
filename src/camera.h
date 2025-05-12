#ifndef CAMERA_H
#define CAMERA_H

#include "vector3f.h"
#include <SFML/System/Vector2.hpp>
#include <cmath> // For std::cos, std::sin

class Camera {
public:
    Camera(const Vector3f& position = Vector3f(0, 0, 0), float yaw = 0.0f, float pitch = 0.0f);
    // 将世界坐标点转换为相机坐标空间
    Vector3f world_to_camera(const Vector3f& world_point) const;

    // 将3D相机坐标点转换为2D屏幕点
    // 注意：这包括一个简单的透视效果和偏移
    sf::Vector2f project(const Vector3f& world_point) const;

    // 旋转相机
    void rotate(float delta_yaw, float delta_pitch);

    // 沿其向前向量移动相机
    void zoom(float amount);

    // 沿视平面移动相机（平移）
    void pan(float delta_x, float delta_y);

    // 获取器
    const Vector3f& get_position() const;
    float get_yaw() const;
    float get_pitch() const;

    void set_position(const Vector3f& position);

private:
    Vector3f position_;
    float yaw_; // 绕Y轴旋转
    float pitch_; // 绕X轴旋转

    // 获取相机的前向、右向和上向量（可能很有用）
    Vector3f get_forward_vector() const;
    Vector3f get_right_vector() const;
    Vector3f get_up_vector() const;
};

#endif // CAMERA_H