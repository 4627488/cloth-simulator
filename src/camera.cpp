#include "camera.h"

Camera::Camera(const Vector3f& position, float yaw, float pitch)
    : position_(position)
    , yaw_(yaw)
    , pitch_(pitch)
{
}

Vector3f Camera::world_to_camera(const Vector3f& world_point) const
{
    // Translate point relative to camera position first
    Vector3f p = world_point - position_;

    // Apply rotations: first pitch (around X), then yaw (around Y)
    float cy = std::cos(yaw_);
    float sy = std::sin(yaw_);
    float cx = std::cos(pitch_);
    float sx = std::sin(pitch_);

    // Original coordinates relative to camera
    float x = p.x;
    float y = p.y;
    float z = p.z;

    // Rotate around Y (yaw)
    float x1 = cy * x + sy * z;
    float z1 = -sy * x + cy * z;

    // Rotate around X (pitch)
    float y2 = cx * y - sx * z1;
    float z2 = sx * y + cx * z1;

    return Vector3f(x1, y2, z2); // Return point in camera coordinates
}

sf::Vector2f Camera::project(const Vector3f& world_point) const
{
    Vector3f cam_coords = world_to_camera(world_point);
    // Simple projection: Offset + scale Z slightly for depth illusion
    // Match the logic from the original project function
    float screen_x = cam_coords.x + 100.0f;
    float screen_y = cam_coords.y + 100.0f - cam_coords.z * 0.5f;
    return sf::Vector2f(screen_x, screen_y);
}

void Camera::rotate(float delta_yaw, float delta_pitch)
{
    yaw_ += delta_yaw;
    pitch_ += delta_pitch;
    // Optional: Clamp pitch to avoid flipping upside down, e.g., +/- PI/2
    // pitch_ = std::clamp(pitch_, -1.5f, 1.5f);
}

void Camera::zoom(float amount)
{
    Vector3f forward = get_forward_vector();
    position_ = position_ + forward * amount;
}

void Camera::pan(float delta_x, float delta_y)
{
    Vector3f right = get_right_vector();
    Vector3f up = get_up_vector();
    position_ = position_ - right * delta_x + up * delta_y; // Note the signs might need adjustment depending on desired pan direction vs mouse movement
}

const Vector3f& Camera::get_position() const
{
    return position_;
}

float Camera::get_yaw() const
{
    return yaw_;
}

float Camera::get_pitch() const
{
    return pitch_;
}

void Camera::set_position(const Vector3f& position)
{
    position_ = position;
}

// Helper function implementations
Vector3f Camera::get_forward_vector() const
{
    float cy = std::cos(yaw_);
    float sy = std::sin(yaw_);
    float cx = std::cos(pitch_);
    float sx = std::sin(pitch_);
    // Forward vector in world space (opposite direction camera is looking)
    // Check the coordinate system assumptions. If Z is depth, forward might be (sy*cx, -sx, cy*cx)
    return Vector3f(sy * cx, -sx, cy * cx).normalized();
}

Vector3f Camera::get_right_vector() const
{
    float cy = std::cos(yaw_);
    float sy = std::sin(yaw_);
    // Right vector is derived from yaw rotation only (lies in XZ plane)
    return Vector3f(cy, 0, -sy).normalized();
}

Vector3f Camera::get_up_vector() const
{
    // Up vector can be derived using cross product of right and forward, or recalculated
    Vector3f forward = get_forward_vector();
    Vector3f right = get_right_vector();
    // Ensure vectors are properly normalized if needed elsewhere
    // Note: This calculation assumes a standard right-handed coordinate system
    // Up = Forward x Right (or Right x Forward depending on handedness/convention)
    // Let's recalculate based on pitch and yaw for consistency
    float cy = std::cos(yaw_);
    float sy = std::sin(yaw_);
    float cx = std::cos(pitch_);
    float sx = std::sin(pitch_);
    return Vector3f(sy * sx, cx, cy * sx).normalized();

    // Alternative using cross product (might be safer if axes are complex):
    // Vector3f world_up(0, 1, 0);
    // Vector3f forward = get_forward_vector();
    // Vector3f right = forward.cross(world_up).normalized();
    // return right.cross(forward).normalized();
}