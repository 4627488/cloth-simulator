#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#include "cloth_state.h"
#include "constraint.h"
#include "input_handler.h"
#include "particle.h"
#include "vector3f.h"

const int WIDTH = 1920;
const int HEIGHT = 1080;
const float PARTICLE_RADIOUS = 10.0f;
const float GRAVITY = 10.0f;
const float TIME_STEP = 0.1f;

const int ROW = 60;
const int COL = 60;
const float REST_DISTANCE = 13.0f;

// 相机参数
float cam_yaw = 0.0f; // 绕Y轴旋转（水平）
float cam_pitch = 0.0f; // 绕X轴旋转（俯仰）
Vector3f cam_pos(0, 0, 0); // 视点原点

// 网格类型枚举
enum class GridType { Square,
    Triangle,
    Hexagon };
GridType grid_type = GridType::Square; // 默认正方形

// 视图变换：世界坐标->相机坐标
Vector3f world_to_camera(const Vector3f& p)
{
    // 先绕X轴旋转pitch，再绕Y轴旋转yaw
    float cy = std::cos(cam_yaw), sy = std::sin(cam_yaw);
    float cx = std::cos(cam_pitch), sx = std::sin(cam_pitch);
    // 旋转矩阵（右手系）
    float x = p.x, y = p.y, z = p.z;
    // 绕Y轴
    float x1 = cy * x + sy * z;
    float z1 = -sy * x + cy * z;
    // 绕X轴
    float y2 = cx * y - sx * z1;
    float z2 = sx * y + cx * z1;
    return Vector3f(x1, y2, z2);
}

void reset_cloth(std::vector<Particle>& particles, std::vector<Constraint>& constraints)
{
    particles.clear();
    constraints.clear();
    if (grid_type == GridType::Square) {
        for (int row = 0; row < ROW; row++) {
            for (int col = 0; col < COL; col++) {
                float x = col * REST_DISTANCE + WIDTH / 6;
                float y = row * REST_DISTANCE + HEIGHT / 6;
                float z = (float)(col + row) / (ROW + COL) * 200.0f + 100.0f; // 简单z扰动
                bool pinned = (row == 0);
                particles.emplace_back(x, y, z, pinned);
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
    } else if (grid_type == GridType::Triangle) {
        // 三角形网格：每个点与右、下、右下、左下相连
        for (int row = 0; row < ROW; row++) {
            for (int col = 0; col < COL; col++) {
                float x = col * REST_DISTANCE + WIDTH / 6;
                float y = row * REST_DISTANCE + HEIGHT / 6;
                float z = (float)(col + row) / (ROW + COL) * 200.0f + 100.0f;
                bool pinned = (row == 0);
                particles.emplace_back(x, y, z, pinned);
            }
        }
        for (int row = 0; row < ROW; row++) {
            for (int col = 0; col < COL; col++) {
                int idx = row * COL + col;
                if (col < COL - 1) // 右
                    constraints.emplace_back(&particles[idx], &particles[idx + 1]);
                if (row < ROW - 1) // 下
                    constraints.emplace_back(&particles[idx], &particles[idx + COL]);
                if (col < COL - 1 && row < ROW - 1) // 右下
                    constraints.emplace_back(&particles[idx], &particles[idx + COL + 1]);
                if (col > 0 && row < ROW - 1) // 左下
                    constraints.emplace_back(&particles[idx], &particles[idx + COL - 1]);
            }
        }
    } else if (grid_type == GridType::Hexagon) {
        // 六边形网格：蜂窝状排列
        float hex_dx = REST_DISTANCE * 0.866f; // cos(30°)
        float hex_dy = REST_DISTANCE * 0.75f; // 行间距
        for (int row = 0; row < ROW; row++) {
            for (int col = 0; col < COL; col++) {
                float x = col * hex_dx + (row % 2) * (hex_dx / 2) + WIDTH / 6;
                float y = row * hex_dy + HEIGHT / 6;
                float z = (float)(col + row) / (ROW + COL) * 200.0f + 100.0f;
                bool pinned = (row == 0);
                particles.emplace_back(x, y, z, pinned);
            }
        }
        for (int row = 0; row < ROW; row++) {
            for (int col = 0; col < COL; col++) {
                int idx = row * COL + col;
                // 右
                if (col < COL - 1)
                    constraints.emplace_back(&particles[idx], &particles[idx + 1]);
                // 下
                if (row < ROW - 1)
                    constraints.emplace_back(&particles[idx], &particles[idx + COL]);
                // 右下
                if (row < ROW - 1 && col < COL - 1 && (row % 2 == 0))
                    constraints.emplace_back(&particles[idx], &particles[idx + COL + 1]);
                // 左下
                if (row < ROW - 1 && col > 0 && (row % 2 == 1))
                    constraints.emplace_back(&particles[idx], &particles[idx + COL - 1]);
            }
        }
    }
}

// 三维投影到二维（带相机）
sf::Vector2f project(const Vector3f& pos)
{
    Vector3f cam = world_to_camera(pos - cam_pos);
    return sf::Vector2f(cam.x + 100, cam.y + 100 - cam.z * 0.5f); // 100偏移防止出界
}

int main()
{
    sf::RenderWindow window(sf::VideoMode({ WIDTH, HEIGHT }), "Cloth Simulation");
    window.setFramerateLimit(60);

    std::vector<Particle> particles;
    std::vector<Constraint> constraints;

    // 拖拽相关变量
    bool dragging = false;
    Particle* dragged_particle = nullptr;

    // 坐标系平移相关变量
    bool panning = false;
    sf::Vector2i pan_start_mouse;
    Vector3f pan_start_cam;

    float wind_strength = 0.0f;
    bool wind_on = false;

    float gravity = GRAVITY;

    bool tear_mode = false;

    reset_cloth(particles, constraints);

    sf::Clock fpsClock;
    float lastFrameTime = fpsClock.getElapsedTime().asSeconds();
    float fps = 0.0f;

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            // X键按下/松开，切换撕裂模式
            if (event->is<sf::Event::KeyPressed>()) {
                auto key = event->getIf<sf::Event::KeyPressed>();
                if (key && key->code == sf::Keyboard::Key::X) {
                    tear_mode = true;
                }
            }
            if (event->is<sf::Event::KeyReleased>()) {
                auto key = event->getIf<sf::Event::KeyReleased>();
                if (key && key->code == sf::Keyboard::Key::X) {
                    tear_mode = false;
                }
            }
            // 鼠标按下，查找最近粒子
            if (event->is<sf::Event::MouseButtonPressed>()) {
                auto mouse = event->getIf<sf::Event::MouseButtonPressed>();
                if (mouse && mouse->button == sf::Mouse::Button::Left) {
                    sf::Vector2i mousePos(mouse->position.x, mouse->position.y);
                    sf::Vector2f mapped = window.mapPixelToCoords(mousePos);
                    Vector3f mousePos3(mapped.x, mapped.y, 0);
                    float minDist = 1e9f;
                    Particle* nearest = nullptr;
                    for (auto& particle : particles) {
                        float dist = (project(particle.position) - sf::Vector2f(mousePos3.x, mousePos3.y)).x * (project(particle.position) - sf::Vector2f(mousePos3.x, mousePos3.y)).x + (project(particle.position) - sf::Vector2f(mousePos3.x, mousePos3.y)).y * (project(particle.position) - sf::Vector2f(mousePos3.x, mousePos3.y)).y;
                        dist = std::sqrt(dist);
                        if (dist < minDist && dist < 30.0f) {
                            minDist = dist;
                            nearest = &particle;
                        }
                    }
                    if (tear_mode && nearest) {
                        // 删除与该粒子相关的所有约束
                        constraints.erase(
                            std::remove_if(constraints.begin(), constraints.end(),
                                [nearest](const Constraint& c) {
                                    return c.p1 == nearest || c.p2 == nearest;
                                }),
                            constraints.end());
                    } else {
                        if (nearest && !nearest->is_pinned)
                            dragging = true, dragged_particle = nearest;
                    }
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
                    sf::Vector2i mousePos(mouse->position.x, mouse->position.y);
                    sf::Vector2f mapped = window.mapPixelToCoords(mousePos);
                    Vector3f mousePos3(mapped.x, mapped.y, 0);
                    float minDist = 1e9f;
                    Particle* nearest = nullptr;
                    for (auto& particle : particles) {
                        float dist = (project(particle.position) - sf::Vector2f(mousePos3.x, mousePos3.y)).x * (project(particle.position) - sf::Vector2f(mousePos3.x, mousePos3.y)).x + (project(particle.position) - sf::Vector2f(mousePos3.x, mousePos3.y)).y * (project(particle.position) - sf::Vector2f(mousePos3.x, mousePos3.y)).y;
                        dist = std::sqrt(dist);
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
            // 鼠标中键按下，开始平移
            if (event->is<sf::Event::MouseButtonPressed>()) {
                auto mouse = event->getIf<sf::Event::MouseButtonPressed>();
                if (mouse && mouse->button == sf::Mouse::Button::Middle) {
                    panning = true;
                    pan_start_mouse = sf::Vector2i(mouse->position.x, mouse->position.y);
                    pan_start_cam = cam_pos;
                }
            }
            // 鼠标中键松开，结束平移
            if (event->is<sf::Event::MouseButtonReleased>()) {
                auto mouse = event->getIf<sf::Event::MouseButtonReleased>();
                if (mouse && mouse->button == sf::Mouse::Button::Middle) {
                    panning = false;
                }
            }
            // 空格键、R键、+/-键、[ / ]键处理
            if (event->is<sf::Event::KeyPressed>()) {
                auto key = event->getIf<sf::Event::KeyPressed>();
                if (key) {
                    // 空格键控制风力
                    if (key->code == sf::Keyboard::Key::Space) {
                        wind_on = !wind_on;
                        wind_strength = wind_on ? 100.0f : 0.0f;
                    }
                    // R键重置布料
                    if (key->code == sf::Keyboard::Key::R) {
                        reset_cloth(particles, constraints);
                    }
                    // +/-键调整重力
                    if (key->code == sf::Keyboard::Key::Equal) {
                        gravity += 1.0f;
                    }
                    if (key->code == sf::Keyboard::Key::Hyphen) {
                        gravity -= 1.0f;
                    }
                    // [ / ]键调整风力
                    if (key->code == sf::Keyboard::Key::LBracket) {
                        wind_strength -= 10.0f;
                    }
                    if (key->code == sf::Keyboard::Key::RBracket) {
                        wind_strength += 10.0f;
                    }
                    // 相机旋转控制
                    if (key->code == sf::Keyboard::Key::A)
                        cam_yaw -= 0.05f;
                    if (key->code == sf::Keyboard::Key::D)
                        cam_yaw += 0.05f;
                    if (key->code == sf::Keyboard::Key::W)
                        cam_pitch -= 0.05f;
                    if (key->code == sf::Keyboard::Key::S)
                        cam_pitch += 0.05f;
                    // T键切换三角形网格
                    if (key->code == sf::Keyboard::Key::T) {
                        grid_type = GridType::Triangle;
                        reset_cloth(particles, constraints);
                    }
                    // H键切换六边形网格
                    if (key->code == sf::Keyboard::Key::H) {
                        grid_type = GridType::Hexagon;
                        reset_cloth(particles, constraints);
                    }
                    // Q键切换正方形网格
                    if (key->code == sf::Keyboard::Key::Q) {
                        grid_type = GridType::Square;
                        reset_cloth(particles, constraints);
                    }
                    // 保存/加载布料状态
                    if (key->code == sf::Keyboard::Key::S && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                        if (ClothState::save(particles, constraints, "cloth_save.txt"))
                            std::cout << "布料已保存到 cloth_save.txt" << std::endl;
                        else
                            std::cout << "保存失败！" << std::endl;
                    }
                    // Ctrl+L 加载
                    if (key->code == sf::Keyboard::Key::L && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                        if (ClothState::load(particles, constraints, "cloth_save.txt"))
                            std::cout << "布料已从 cloth_save.txt 加载" << std::endl;
                        else
                            std::cout << "加载失败！" << std::endl;
                    }
                }
            }
            // 鼠标滚轮控制相机前后移动
            if (event->is<sf::Event::MouseWheelScrolled>()) {
                auto wheel = event->getIf<sf::Event::MouseWheelScrolled>();
                if (wheel) {
                    // 计算相机前向向量
                    float cy = std::cos(cam_yaw), sy = std::sin(cam_yaw);
                    float cx = std::cos(cam_pitch), sx = std::sin(cam_pitch);
                    Vector3f forward(
                        sy * cx,
                        -sx,
                        cy * cx);
                    float zoomSpeed = 50.0f;
                    cam_pos = cam_pos + forward * (wheel->delta * zoomSpeed);
                }
            }
            // 其他事件
            InputHandler::handle_mouse_click(*event, particles, constraints);
        }

        // 拖拽时让粒子跟随鼠标
        if (dragging && dragged_particle) {
            // 全屏下用 mapPixelToCoords 保证坐标正确
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            sf::Vector2f mapped = window.mapPixelToCoords(mousePos);
            dragged_particle->position.x = mapped.x;
            dragged_particle->position.y = mapped.y + dragged_particle->position.z * 0.5f;
        }

        // 鼠标中键拖动时，实时平移
        if (panning) {
            sf::Vector2i cur_mouse = sf::Mouse::getPosition(window);
            sf::Vector2f delta = sf::Vector2f(cur_mouse - pan_start_mouse);
            // 计算相机的右和上方向
            float cy = std::cos(cam_yaw), sy = std::sin(cam_yaw);
            float cx = std::cos(cam_pitch), sx = std::sin(cam_pitch);
            // 相机右方向（世界坐标下）
            Vector3f right = Vector3f(cy, 0, -sy);
            // 相机上方向（世界坐标下）
            Vector3f up = Vector3f(sy * sx, cx, cy * sx);
            // 鼠标x映射到right，y映射到up
            cam_pos = pan_start_cam - right * delta.x - up * delta.y;
        }

        // apply gravity and update particles
        for (auto& particle : particles) {
            particle.apply_force(Vector3f(0, gravity, 0));
            if (wind_on) {
                particle.apply_force(Vector3f(wind_strength, 0, 0));
            }
            particle.update(TIME_STEP);
            particle.constrain_to_bounds(WIDTH, HEIGHT, 1000.0f);
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
            sf::Color color = particle.is_pinned ? sf::Color::Red : sf::Color(255, 255 - (int)(particle.position.y / HEIGHT * 255), 255 - (int)(particle.position.z / 1000.0f * 255));
            sf::Vertex point { project(particle.position), color };
            window.draw(&point, 1, sf::PrimitiveType::Points);
        }

        // Draw constraints as lines
        for (const auto& constraint : constraints) {
            if (!constraint.active) {
                continue;
            }
            float len = (constraint.p1->position - constraint.p2->position).length();
            float t = std::min(std::abs(len - constraint.initial_length) / (constraint.initial_length * 0.5f), 1.0f);
            sf::Color lineColor = sf::Color(255, (uint8_t)(255 * (1 - t)), (uint8_t)(255 * (1 - t)));
            sf::Vertex line[] = {
                { project(constraint.p1->position), lineColor },
                { project(constraint.p2->position), lineColor },
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }

        // 绘制左上角相机参考系
        static sf::Font font;
        static bool font_loaded = false;
        if (!font_loaded) {
            font_loaded = font.openFromFile("Arial.ttf");
            if (!font_loaded)
                font_loaded = font.openFromFile("fonts/Arial.ttf");
#if defined(_WIN32)
            if (!font_loaded)
                font_loaded = font.openFromFile("C:/Windows/Fonts/Arial.ttf");
#elif defined(__APPLE__)
            if (!font_loaded)
                font_loaded = font.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf");
#elif defined(__linux__)
            if (!font_loaded)
                font_loaded = font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif
            if (!font_loaded) {
                std::cerr << "字体加载失败，请将 Arial.ttf 放到程序目录或 fonts 目录下！" << std::endl;
            }
        }
        // 帧率统计（实时更新）
        float currentTime = fpsClock.getElapsedTime().asSeconds();
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        if (deltaTime > 0.0001f) {
            const float alpha = 0.1f;
            fps = alpha * (1.0f / deltaTime) + (1 - alpha) * fps;
        }
        if (font_loaded) {
            std::string info_str = "Points: " + std::to_string(particles.size()) + "\nConstraints: " + std::to_string(constraints.size()) + "\nFPS: " + std::to_string(fps) + "\nTear Mode: " + (tear_mode ? "ON" : "OFF") + "\nWind Mode: " + (wind_on ? "ON" : "OFF");
            info_str += "\nGrid: ";
            if (grid_type == GridType::Square)
                info_str += "Square";
            else if (grid_type == GridType::Triangle)
                info_str += "Triangle";
            else if (grid_type == GridType::Hexagon)
                info_str += "Hexagon";
            sf::Text info(font, info_str);
            info.setFillColor(sf::Color::White);
            info.setPosition(sf::Vector2f(static_cast<float>(WIDTH - 350), 20.f)); // 右上角
            window.draw(info);
            // 绘制操作说明（右下角，英文）
            std::string help_str = "Left click: Drag particle\n"
                                   "Right click: Pin/unpin\n"
                                   "Middle click: Pan\n"
                                   "Mouse wheel: Zoom\n"
                                   "R: Reset cloth\n"
                                   "+/-: Gravity\n"
                                   "[ ]: Wind\n"
                                   "WASD: Rotate view\n"
                                   "Space: Toggle wind\n"
                                   "T: Triangle grid\n"
                                   "H: Hex grid\n"
                                   "Q: Square grid\n"
                                   "Ctrl+S: Save cloth\n"
                                   "Ctrl+L: Load cloth";
            sf::Text help(font, help_str);
            help.setFillColor(sf::Color(200, 200, 200));
            help.setCharacterSize(22);
            auto helpBounds = help.getLocalBounds();
            float helpWidth = helpBounds.size.x;
            float helpHeight = helpBounds.size.y;
            help.setPosition(sf::Vector2f(WIDTH - helpWidth - 40.f, HEIGHT - helpHeight - 80.f));
            window.draw(help);
            // 绘制坐标轴（加粗版）
            sf::Vector2f origin(80, 120);
            float axis_len = 50;
            float thick = 4.0f; // 线宽
            // 相机当前的三个轴
            Vector3f x_axis = world_to_camera(Vector3f(1, 0, 0)).normalized();
            Vector3f y_axis = world_to_camera(Vector3f(0, 1, 0)).normalized();
            Vector3f z_axis = world_to_camera(Vector3f(0, 0, 1)).normalized();
            auto draw_thick_line = [&](const sf::Vector2f& from, const sf::Vector2f& to, sf::Color color) {
                sf::Vector2f dir = to - from;
                float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                if (len < 1e-3f)
                    return;
                dir /= len;
                sf::Vector2f normal(-dir.y, dir.x);
                sf::Vector2f p0 = from + normal * (thick / 2.f);
                sf::Vector2f p1 = from - normal * (thick / 2.f);
                sf::Vector2f p2 = to - normal * (thick / 2.f);
                sf::Vector2f p3 = to + normal * (thick / 2.f);
                sf::Vertex tri[6] = {
                    { p0, color }, { p1, color }, { p2, color },
                    { p0, color }, { p2, color }, { p3, color }
                };
                window.draw(tri, 6, sf::PrimitiveType::Triangles);
            };
            draw_thick_line(origin, origin + sf::Vector2f(x_axis.x, x_axis.y) * axis_len, sf::Color::Red);
            draw_thick_line(origin, origin + sf::Vector2f(y_axis.x, y_axis.y) * axis_len, sf::Color::Green);
            draw_thick_line(origin, origin + sf::Vector2f(z_axis.x, z_axis.y) * axis_len, sf::Color::Blue);
        }

        window.display();
    }
}