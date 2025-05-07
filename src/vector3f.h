#ifndef VECTOR3F_H
#define VECTOR3F_H
#include <cmath>

struct Vector3f {
    float x, y, z;
    Vector3f(float x_ = 0, float y_ = 0, float z_ = 0)
        : x(x_)
        , y(y_)
        , z(z_)
    {
    }
    Vector3f operator+(const Vector3f& rhs) const { return Vector3f(x + rhs.x, y + rhs.y, z + rhs.z); }
    Vector3f operator-(const Vector3f& rhs) const { return Vector3f(x - rhs.x, y - rhs.y, z - rhs.z); }
    Vector3f& operator+=(const Vector3f& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
    Vector3f& operator-=(const Vector3f& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }
    Vector3f operator*(float s) const { return Vector3f(x * s, y * s, z * s); }
    Vector3f& operator*=(float s)
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
    float dot(const Vector3f& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    void set(float x_, float y_, float z_)
    {
        x = x_;
        y = y_;
        z = z_;
    }
    Vector3f cross(const Vector3f& rhs) const { return Vector3f(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x); }
    Vector3f normalized() const
    {
        float len = length();
        return len > 0 ? (*this) * (1.0f / len) : Vector3f(0, 0, 0);
    }
};

#endif // VECTOR3F_H