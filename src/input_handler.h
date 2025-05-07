#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "constraint.h"
#include "particle.h"
#include "vector3f.h"
#include <SFML/Graphics.hpp>
#include <vector>

const float CLICK_TOLERANCE = 5.0f;

class InputHandler {
public:
    static void handle_mouse_click(const sf::Event& event, std::vector<Particle>& particles,
        std::vector<Constraint>& constraints)
    {
        if (event.is<sf::Event::MouseButtonPressed>()) {
            const auto* mouse = event.getIf<sf::Event::MouseButtonPressed>();
            if (mouse && mouse->button == sf::Mouse::Button::Left) {
                float mouse_x = static_cast<float>(mouse->position.x);
                float mouse_y = static_cast<float>(mouse->position.y);
                // 假设投影到z=0平面
                tear_cloth(Vector3f(mouse_x, mouse_y, 0), particles, constraints);
            }
        }
    }

private:
    static float point_to_segment_distance(const Vector3f& p, const Vector3f& a, const Vector3f& b)
    {
        Vector3f ab = b - a;
        Vector3f ap = p - a;
        float ab_len2 = ab.dot(ab);
        float t = ab_len2 > 0 ? ap.dot(ab) / ab_len2 : 0;
        t = std::max(0.0f, std::min(1.0f, t));
        Vector3f proj = a + ab * t;
        return (p - proj).length();
    }

    static Constraint* find_nearest_constraint(const Vector3f& mouse_pos,
        const std::vector<Constraint>& constraints)
    {
        Constraint* nearest_constraint = nullptr;
        float min_distance = CLICK_TOLERANCE;

        for (const auto& constraint : constraints) {
            float distance = point_to_segment_distance(mouse_pos,
                constraint.p1->position, constraint.p2->position);
            if (distance < min_distance) {
                min_distance = distance;
                nearest_constraint = const_cast<Constraint*>(&constraint);
            }
        }
        return nearest_constraint;
    }

    static void tear_cloth(const Vector3f& mouse_pos, const std::vector<Particle>& particles,
        std::vector<Constraint>& constraints)
    {
        Constraint* nearest = find_nearest_constraint(mouse_pos, constraints);
        if (nearest) {
            nearest->deactivate();
        }
    }
};

#endif // INPUT_HANDLER_H