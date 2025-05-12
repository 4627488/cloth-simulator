#ifndef CAMERA_H
#define CAMERA_H

#include "vector3f.h"
#include <SFML/System/Vector2.hpp>
#include <cmath> // For std::cos, std::sin

class Camera {
public:
    Camera(const Vector3f& position = Vector3f(0, 0, 0), float yaw = 0.0f, float pitch = 0.0f);

    // Transforms a world coordinate point to camera coordinate space
    Vector3f world_to_camera(const Vector3f& world_point) const;

    // Projects a 3D camera coordinate point to a 2D screen point
    // Note: This includes a simple perspective effect and offset
    sf::Vector2f project(const Vector3f& world_point) const;

    // Rotates the camera
    void rotate(float delta_yaw, float delta_pitch);

    // Moves the camera along its forward vector
    void zoom(float amount);

    // Moves the camera parallel to the view plane (panning)
    void pan(float delta_x, float delta_y);

    // Getters
    const Vector3f& get_position() const;
    float get_yaw() const;
    float get_pitch() const;

    // Setters (optional, depending on design)
    void set_position(const Vector3f& position);

private:
    Vector3f position_;
    float yaw_; // Rotation around Y-axis
    float pitch_; // Rotation around X-axis

    // Helper to get camera's forward, right, and up vectors might be useful
    Vector3f get_forward_vector() const;
    Vector3f get_right_vector() const;
    Vector3f get_up_vector() const;
};

#endif // CAMERA_H