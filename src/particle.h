#ifndef PARTICLE_H
#define PARTICLE_H

#include "vector3f.h"

class Particle {
public:
    Vector3f position;
    Vector3f previous_position;
    Vector3f acceleration;
    bool is_pinned;

    Particle(float x, float y, float z = 0, bool pinned = false)
        : position(x, y, z)
        , previous_position(x, y, z)
        , acceleration(0, 0, 0)
        , is_pinned(pinned)
    {
    }

    void apply_force(const Vector3f& force)
    {
        if (!is_pinned) {
            acceleration += force;
        }
    }

    void update(float time_step)
    {
        // verlet integration
        if (!is_pinned) {
            Vector3f velocity = position - previous_position;
            previous_position = position;
            position += velocity + acceleration * (time_step * time_step);
            acceleration = Vector3f(0, 0, 0); // reset after update
        }
    }

    void constrain_to_bounds(float width, float height, float depth = 1000.0f)
    {
        if (position.x < 0)
            position.x = 0;
        if (position.x > width)
            position.x = width;
        if (position.y < 0)
            position.y = 0;
        if (position.y > height)
            position.y = height;
        if (position.z < 0)
            position.z = 0;
        if (position.z > depth)
            position.z = depth;
    }
};

#endif // PARTICLE_H