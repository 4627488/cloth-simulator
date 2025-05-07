#pragma once
#include "constraint.h"
#include "particle.h"
#include <SFML/Graphics.hpp>
#include <vector>

// 布料模拟类，封装粒子、约束及相关操作
class Cloth {
public:
    Cloth(int row, int col, float rest_distance, float width, float height);
    void reset(); // 重置布料
    void update(float gravity, float wind, float time_step, int satisfy_iter); // 更新物理状态
    void draw(sf::RenderWindow& window); // 绘制布料

    // 交互操作
    void apply_drag(const sf::Vector2f& pos); // 拖拽粒子
    void stop_drag(); // 停止拖拽
    void toggle_pin(const sf::Vector2f& pos); // 固定/解固定粒子

    // 获取最近粒子（用于交互）
    Particle* get_nearest_particle(const sf::Vector2f& pos, float radius = 30.0f);

    // 参数设置
    void set_wind(float w) { wind_strength = w; }
    void set_gravity(float g) { gravity = g; }

    // 只读访问（如需外部遍历）
    const std::vector<Particle>& get_particles() const { return particles; }
    const std::vector<Constraint>& get_constraints() const { return constraints; }

private:
    int row, col; // 行列数
    float rest_distance; // 粒子间距
    float width, height; // 区域尺寸
    float wind_strength = 0.0f;
    float gravity = 10.0f;

    std::vector<Particle> particles;
    std::vector<Constraint> constraints;
    Particle* dragged_particle = nullptr;

    void init_particles(); // 初始化粒子
    void init_constraints(); // 初始化约束

    // 辅助：计算粒子初始位置偏移
    float get_x_offset() const;
    float get_y_offset() const;
};