#include "cloth.h"
#include <algorithm>
#include <cmath>

// 粒子初始位置偏移，便于布料居中
float Cloth::get_x_offset() const
{
    return width / 6.0f;
}
float Cloth::get_y_offset() const
{
    return height / 6.0f;
}

Cloth::Cloth(int row_, int col_, float rest_distance_, float width_, float height_)
    : row(row_)
    , col(col_)
    , rest_distance(rest_distance_)
    , width(width_)
    , height(height_)
{
    reset();
}

// 初始化所有粒子
void Cloth::init_particles()
{
    particles.clear();
    float x_offset = get_x_offset();
    float y_offset = get_y_offset();
    for (int r = 0; r < row; ++r) {
        for (int c = 0; c < col; ++c) {
            float x = c * rest_distance + x_offset;
            float y = r * rest_distance + y_offset;
            bool pinned = (r == 0); // 顶部粒子固定
            particles.emplace_back(x, y, pinned);
        }
    }
}

// 初始化所有约束
void Cloth::init_constraints()
{
    constraints.clear();
    for (int r = 0; r < row; ++r) {
        for (int c = 0; c < col; ++c) {
            int idx = r * col + c;
            if (c < col - 1) {
                constraints.emplace_back(&particles[idx], &particles[idx + 1]);
            }
            if (r < row - 1) {
                constraints.emplace_back(&particles[idx], &particles[idx + col]);
            }
        }
    }
}

void Cloth::reset()
{
    init_particles();
    init_constraints();
    dragged_particle = nullptr;
}

// 更新物理状态
void Cloth::update(float gravity_, float wind, float time_step, int satisfy_iter)
{
    gravity = gravity_;
    // 施加重力和风力
    for (auto& p : particles) {
        p.apply_force(sf::Vector2f(wind, gravity));
        p.update(time_step);
        p.constrain_to_bounds(width, height);
    }
    // 约束迭代
    for (int i = 0; i < satisfy_iter; ++i) {
        for (auto& c : constraints) {
            c.satisfy();
        }
    }
}

// 计算粒子颜色
static sf::Color get_particle_color(const Particle& p, float height)
{
    if (p.is_pinned)
        return sf::Color::Red;
    int green = 255 - static_cast<int>(p.position.y / height * 255);
    return sf::Color(255, std::clamp(green, 0, 255), 255);
}

// 计算约束线颜色
static sf::Color get_constraint_color(const Constraint& c)
{
    float len = std::hypot(c.p1->position.x - c.p2->position.x, c.p1->position.y - c.p2->position.y);
    float t = std::min(std::abs(len - c.initial_length) / (c.initial_length * 0.5f), 1.0f);
    uint8_t color_val = static_cast<uint8_t>(255 * (1 - t));
    return sf::Color(255, color_val, color_val);
}

// 绘制布料
void Cloth::draw(sf::RenderWindow& window)
{
    // 画粒子
    for (const auto& p : particles) {
        sf::Vertex point { p.position, get_particle_color(p, height) };
        window.draw(&point, 1, sf::PrimitiveType::Points);
    }
    // 画约束
    for (const auto& c : constraints) {
        if (!c.active)
            continue;
        sf::Color lineColor = get_constraint_color(c);
        sf::Vertex line[] = {
            { c.p1->position, lineColor },
            { c.p2->position, lineColor },
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

// 查找最近粒子
Particle* Cloth::get_nearest_particle(const sf::Vector2f& pos, float radius)
{
    auto it = std::min_element(particles.begin(), particles.end(), [&](const Particle& a, const Particle& b) {
        float da = std::hypot(a.position.x - pos.x, a.position.y - pos.y);
        float db = std::hypot(b.position.x - pos.x, b.position.y - pos.y);
        return da < db;
    });
    if (it != particles.end() && std::hypot(it->position.x - pos.x, it->position.y - pos.y) < radius) {
        return &(*it);
    }
    return nullptr;
}

// 拖拽粒子
void Cloth::apply_drag(const sf::Vector2f& pos)
{
    dragged_particle = get_nearest_particle(pos);
    if (dragged_particle && !dragged_particle->is_pinned) {
        dragged_particle->position = pos;
    }
}

void Cloth::stop_drag()
{
    dragged_particle = nullptr;
}

// 固定/解固定粒子
void Cloth::toggle_pin(const sf::Vector2f& pos)
{
    Particle* p = get_nearest_particle(pos);
    if (p)
        p->is_pinned = !p->is_pinned;
}