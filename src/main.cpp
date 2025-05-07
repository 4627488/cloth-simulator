#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cstdint>
#include <iostream>
#include <vector>

#include "constraint.h"
#include "input_handler.h"
#include "particle.h"

const int WIDTH = 2000;
const int HEIGHT = 2000;
const float PARTICLE_RADIOUS = 10.0f;
const float GRAVITY = 10.0f;
const float TIME_STEP = 0.1f;

const int ROW = 60;
const int COL = 60;
const float REST_DISTANCE = 20.0f;

// 键值常量（如有需要可根据实际环境调整）
#define KEY_SPACE 57
#define KEY_R 27
#define KEY_EQUAL 36
#define KEY_HYPHEN 45
#define KEY_LBRACKET 47
#define KEY_RBRACKET 48

int main()
{
    sf::RenderWindow window(sf::VideoMode({ WIDTH, HEIGHT }), "Cloth Simulation");
    window.setFramerateLimit(60);

    std::vector<Particle> particles;
    std::vector<Constraint> constraints;

    // 拖拽相关变量
    bool dragging = false;
    Particle* dragged_particle = nullptr;

    float wind_strength = 0.0f;
    bool wind_on = false;

    float gravity = GRAVITY;

    for (int row = 0; row < ROW; row++) {
        for (int col = 0; col < COL; col++) {
            float x = col * REST_DISTANCE + WIDTH / 6;
            float y = row * REST_DISTANCE + HEIGHT / 6;
            bool pinned = (row == 0);
            particles.emplace_back(x, y, pinned);
        }
    }

    // Initialize constraints
    for (int row = 0; row < ROW; row++) {
        for (int col = 0; col < COL; col++) {
            if (col < COL - 1) {
                // Horizontal constraint
                constraints.emplace_back(&particles[row * COL + col], &particles[row * COL + col + 1]);
            }
            if (row < ROW - 1) {
                // Vertical constraint
                constraints.emplace_back(&particles[row * COL + col], &particles[(row + 1) * COL + col]);
            }
        }
    }

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            // 鼠标按下，查找最近粒子
            if (event->is<sf::Event::MouseButtonPressed>()) {
                auto mouse = event->getIf<sf::Event::MouseButtonPressed>();
                if (mouse && mouse->button == sf::Mouse::Button::Left) {
                    auto mousePos = sf::Mouse::getPosition(window);
                    sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
                    float minDist = 1e9f;
                    for (auto& particle : particles) {
                        float dist = std::hypot(particle.position.x - mousePosF.x, particle.position.y - mousePosF.y);
                        if (dist < minDist && dist < 30.0f) { // 30.0f为拖拽半径
                            minDist = dist;
                            dragged_particle = &particle;
                        }
                    }
                    if (dragged_particle && !dragged_particle->is_pinned)
                        dragging = true;
                }
            }
            // 鼠标释放，取消拖拽
            if (event->is<sf::Event::MouseButtonReleased>()) {
                auto mouse = event->getIf<sf::Event::MouseButtonReleased>();
                if (mouse && mouse->button == sf::Mouse::Button::Left) {
                    dragging = false;
                    dragged_particle = nullptr;
                }
            }
            // 鼠标右键切换粒子固定状态
            if (event->is<sf::Event::MouseButtonPressed>()) {
                auto mouse = event->getIf<sf::Event::MouseButtonPressed>();
                if (mouse && mouse->button == sf::Mouse::Button::Right) {
                    auto mousePos = sf::Mouse::getPosition(window);
                    sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
                    float minDist = 1e9f;
                    Particle* nearest = nullptr;
                    for (auto& particle : particles) {
                        float dist = std::hypot(particle.position.x - mousePosF.x, particle.position.y - mousePosF.y);
                        if (dist < minDist && dist < 30.0f) {
                            minDist = dist;
                            nearest = &particle;
                        }
                    }
                    if (nearest) {
                        nearest->is_pinned = !nearest->is_pinned;
                    }
                }
            }
            // 空格键、R键、+/-键、[ / ]键处理
            if (event->is<sf::Event::KeyPressed>()) {
                auto key = event->getIf<sf::Event::KeyPressed>();
                if (key) {
                    // 空格键控制风力
                    if ((int)key->code == KEY_SPACE) {
                        wind_on = !wind_on;
                        wind_strength = wind_on ? 100.0f : 0.0f;
                    }
                    // R键重置布料
                    if ((int)key->code == KEY_R) {
                        particles.clear();
                        constraints.clear();
                        for (int row = 0; row < ROW; row++) {
                            for (int col = 0; col < COL; col++) {
                                float x = col * REST_DISTANCE + WIDTH / 6;
                                float y = row * REST_DISTANCE + HEIGHT / 6;
                                bool pinned = (row == 0);
                                particles.emplace_back(x, y, pinned);
                            }
                        }
                        for (int row = 0; row < ROW; row++) {
                            for (int col = 0; col < COL; col++) {
                                if (col < COL - 1) {
                                    constraints.emplace_back(&particles[row * COL + col], &particles[row * COL + col + 1]);
                                }
                                if (row < ROW - 1) {
                                    constraints.emplace_back(&particles[row * COL + col], &particles[(row + 1) * COL + col]);
                                }
                            }
                        }
                    }
                    // +/-键调整重力
                    if ((int)key->code == KEY_EQUAL) {
                        gravity += 1.0f;
                    }
                    if ((int)key->code == KEY_HYPHEN) {
                        gravity -= 1.0f;
                    }
                    // [ / ]键调整风力
                    if ((int)key->code == KEY_LBRACKET) {
                        wind_strength -= 10.0f;
                    }
                    if ((int)key->code == KEY_RBRACKET) {
                        wind_strength += 10.0f;
                    }
                }
            }
            // 其他事件
            InputHandler::handle_mouse_click(*event, particles, constraints);
        }

        // 拖拽时让粒子跟随鼠标
        if (dragging && dragged_particle) {
            auto mousePos = sf::Mouse::getPosition(window);
            dragged_particle->position = sf::Vector2f(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
            // 也可以用apply_force实现更自然的拉扯
        }

        // apply gravity and update particles
        for (auto& particle : particles) {
            particle.apply_force(sf::Vector2f(0, gravity));
            if (wind_on) {
                particle.apply_force(sf::Vector2f(wind_strength, 0));
            }
            particle.update(TIME_STEP);
            particle.constrain_to_bounds(WIDTH, HEIGHT);
        }

        for (size_t i = 0; i < 5; i++) {
            for (auto& constraint : constraints) {
                constraint.satisfy();
            }
        }

        window.clear(sf::Color::Black);

        // Draw particles as balls
        // for (const auto& particle : particles) {
        //     sf::CircleShape circle(PARTICLE_RADIOUS);
        //     circle.setFillColor(sf::Color::White);
        //     circle.setPosition(particle.position.x - PARTICLE_RADIOUS,
        //                         particle.position.y - PARTICLE_RADIOUS);
        //     window.draw(circle);
        // }

        // Draw particles as points
        for (const auto& particle : particles) {
            // 颜色渐变：y越大越蓝，固定点为红色
            sf::Color color = particle.is_pinned ? sf::Color::Red : sf::Color(255, 255 - (int)(particle.position.y / HEIGHT * 255), 255);
            sf::Vertex point { particle.position, color };
            window.draw(&point, 1, sf::PrimitiveType::Points);
        }

        // Draw constraints as lines
        for (const auto& constraint : constraints) {
            if (!constraint.active) {
                continue;
            }
            // 线条颜色根据长度变化（受力大为红色，正常为白色）
            float len = std::hypot(constraint.p1->position.x - constraint.p2->position.x, constraint.p1->position.y - constraint.p2->position.y);
            float t = std::min(std::abs(len - constraint.initial_length) / (constraint.initial_length * 0.5f), 1.0f);
            sf::Color lineColor = sf::Color(255, (uint8_t)(255 * (1 - t)), (uint8_t)(255 * (1 - t)));
            sf::Vertex line[] = {
                { constraint.p1->position, lineColor },
                { constraint.p2->position, lineColor },
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }

        window.display();
    }
}