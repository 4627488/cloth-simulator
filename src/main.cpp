#define _USE_MATH_DEFINES // For M_PI on Windows
#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>

#include "cloth_state.h"
#include "constants.h"
#include "constraint.h"
#include "input_handler.h"
#include "particle.h"
#include "vector3f.h"

// 相机参数
float cam_yaw = 0.0f; // 绕Y轴旋转（水平）
float cam_pitch = 0.0f; // 绕X轴旋转（俯仰）
Vector3f cam_pos; // 视点原点 - 将由 update_camera_position 计算
float cam_distance = 800.0f; // 相机距离原点的距离
const float PITCH_LIMIT = M_PI / 2.0f - 0.01f; // 限制俯仰角防止万向节死锁/翻转
const float fov_factor = 600.0f; // 视野/焦距因子 for projection

// 网格类型枚举
enum class GridType { Square,
    Triangle,
    Hexagon };
GridType grid_type = GridType::Square; // 默认正方形

// 函数：根据角度和距离更新相机位置
void update_camera_position()
{
    // 限制俯仰角
    if (cam_pitch > PITCH_LIMIT)
        cam_pitch = PITCH_LIMIT;
    if (cam_pitch < -PITCH_LIMIT)
        cam_pitch = -PITCH_LIMIT;

    // 计算相机在以原点为中心的球面上的位置 (Y轴向上)
    cam_pos.x = cam_distance * std::sin(cam_yaw) * std::cos(cam_pitch);
    cam_pos.y = cam_distance * std::sin(cam_pitch); // 正俯仰角使相机向上移动
    cam_pos.z = cam_distance * std::cos(cam_yaw) * std::cos(cam_pitch); // Z轴在 yaw=0, pitch=0 时向前
}

// 视图变换：世界坐标 -> 相机坐标 (使用 LookAt 方法)
Vector3f world_to_camera(const Vector3f& p)
{
    Vector3f target(0.0f, 0.0f, 0.0f); // 目标点：原点
    Vector3f world_up(0.0f, 1.0f, 0.0f); // 世界坐标系的上方向

    // 1. 计算相机方向向量 (基于当前 cam_pos)
    Vector3f zaxis = (cam_pos - target).normalized(); // 相机看向目标的反方向 (-Z in view space usually)
    Vector3f xaxis = world_up.cross(zaxis).normalized(); // 相机的右方向 (X)
    Vector3f yaxis = zaxis.cross(xaxis); // 相机的上方向 (Y)

    // 2. 计算点 p 相对于相机位置的向量
    Vector3f relative_p = p - cam_pos;

    // 3. 将 relative_p 投影到相机坐标轴上得到相机坐标
    float cam_x = relative_p.dot(xaxis);
    float cam_y = relative_p.dot(yaxis);
    float cam_z = relative_p.dot(zaxis); // Z 轴通常指向相机后方

    // 返回相机坐标系中的坐标 (通常约定相机看向 -Z 方向)
    return Vector3f(cam_x, cam_y, -cam_z);
}

// 投影：相机坐标 -> 二维屏幕坐标 (简单透视)
sf::Vector2f project(const Vector3f& world_pos)
{
    Vector3f cam_coords = world_to_camera(world_pos);

    // 简单的透视投影
    float perspective_z = cam_coords.z; // Z 值代表深度（相机坐标系下，正值在相机前方）

    // 防止除以零或负数 (点在相机后方)
    if (perspective_z <= 0.1f) {
        // perspective_z = 0.1f; // Clamp near plane
        // 或者直接将点移出屏幕
        return sf::Vector2f(-10000, -10000);
    }

    // 透视除法，并将原点移到屏幕中心
    float screen_x = (cam_coords.x * fov_factor / perspective_z) + WIDTH / 2.f;
    float screen_y = (-cam_coords.y * fov_factor / perspective_z) + HEIGHT / 2.f; // Y 轴翻转以匹配屏幕坐标

    return sf::Vector2f(screen_x, screen_y);
}

void reset_cloth(std::vector<Particle>& particles, std::vector<Constraint>& constraints)
{
    particles.clear();
    constraints.clear();
    int rows_to_use = DEFAULT_ROW;
    int cols_to_use = DEFAULT_COL;
    float rest_distance_to_use = DEFAULT_REST_DISTANCE;

    if (grid_type == GridType::Square) {
        for (int row = 0; row < rows_to_use; row++) {
            for (int col = 0; col < cols_to_use; col++) {
                float x = col * rest_distance_to_use - WIDTH / 6;
                float y = row * rest_distance_to_use + HEIGHT / 6;
                float z = (float)(col + row) / (rows_to_use + cols_to_use) * 200.0f + 100.0f; // 简单z扰动
                bool pinned = (row == 0);
                particles.emplace_back(x, y, z, pinned);
            }
        }
        for (int row = 0; row < rows_to_use; row++) {
            for (int col = 0; col < cols_to_use; col++) {
                if (col < cols_to_use - 1) {
                    constraints.emplace_back(&particles[row * cols_to_use + col], &particles[row * cols_to_use + col + 1], rest_distance_to_use);
                }
                if (row < rows_to_use - 1) {
                    constraints.emplace_back(&particles[row * cols_to_use + col], &particles[(row + 1) * cols_to_use + col], rest_distance_to_use);
                }
            }
        }
    } else if (grid_type == GridType::Triangle) {
        // 三角形网格：每个点与右、下、右下、左下相连
        for (int row = 0; row < rows_to_use; row++) {
            for (int col = 0; col < cols_to_use; col++) {
                float x = col * rest_distance_to_use - WIDTH / 6;
                float y = row * rest_distance_to_use + HEIGHT / 6;
                float z = (float)(col + row) / (rows_to_use + cols_to_use) * 200.0f + 100.0f;
                bool pinned = (row == 0);
                particles.emplace_back(x, y, z, pinned);
            }
        }
        for (int row = 0; row < rows_to_use; row++) {
            for (int col = 0; col < cols_to_use; col++) {
                int idx = row * cols_to_use + col;
                if (col < cols_to_use - 1) // 右
                    constraints.emplace_back(&particles[idx], &particles[idx + 1], rest_distance_to_use);
                if (row < rows_to_use - 1) // 下
                    constraints.emplace_back(&particles[idx], &particles[idx + cols_to_use], rest_distance_to_use);
                if (col < cols_to_use - 1 && row < rows_to_use - 1) // 右下
                    constraints.emplace_back(&particles[idx], &particles[idx + cols_to_use + 1], rest_distance_to_use * std::sqrt(2.f)); // Diagonal
                if (col > 0 && row < rows_to_use - 1) // 左下
                    constraints.emplace_back(&particles[idx], &particles[idx + cols_to_use - 1], rest_distance_to_use * std::sqrt(2.f)); // Diagonal
            }
        }
    } else if (grid_type == GridType::Hexagon) {
        // 六边形网格：蜂窝状排列
        float hex_dx = rest_distance_to_use * 0.866f; // cos(30°)
        float hex_dy = rest_distance_to_use * 0.75f; // 行间距
        for (int row = 0; row < rows_to_use; row++) {
            for (int col = 0; col < cols_to_use; col++) {
                float x = col * hex_dx + (row % 2) * (hex_dx / 2) + WIDTH / 6;
                float y = row * hex_dy + HEIGHT / 6;
                float z = (float)(col + row) / (rows_to_use + cols_to_use) * 200.0f + 100.0f;
                bool pinned = (row == 0);
                particles.emplace_back(x, y, z, pinned);
            }
        }
        for (int row = 0; row < rows_to_use; row++) {
            for (int col = 0; col < cols_to_use; col++) {
                int idx = row * cols_to_use + col;
                // 水平方向连接 (右)
                if (col < cols_to_use - 1)
                    constraints.emplace_back(&particles[idx], &particles[idx + 1], hex_dx);

                // 斜向下连接 (考虑奇偶行)
                if (row < rows_to_use - 1) {
                    // 奇数行: 左下和右下
                    if (row % 2 == 1) {
                        if (col > 0)
                            constraints.emplace_back(&particles[idx], &particles[idx + cols_to_use - 1], rest_distance_to_use); // 左下
                        constraints.emplace_back(&particles[idx], &particles[idx + cols_to_use], rest_distance_to_use); // 正下 (近似)
                    }
                    // 偶数行: 左下和右下
                    else {
                        constraints.emplace_back(&particles[idx], &particles[idx + cols_to_use], rest_distance_to_use); // 正下 (近似)
                        if (col < cols_to_use - 1)
                            constraints.emplace_back(&particles[idx], &particles[idx + cols_to_use + 1], rest_distance_to_use); // 右下
                    }
                }
            }
        }
    }
    for (auto& c : constraints) {
        if (c.initial_length <= 0) {
            // Temporary fix: use default rest distance if calculation isn't straightforward here
        }
    }
}

int main()
{
    sf::RenderWindow window(sf::VideoMode({ (unsigned int)WIDTH, (unsigned int)HEIGHT }), "Cloth Simulation");
    window.setFramerateLimit(60);

    // 初始化相机位置
    update_camera_position();

    std::vector<Particle> particles;
    std::vector<Constraint> constraints;

    // 拖拽相关变量
    bool dragging = false;
    Particle* dragged_particle = nullptr;
    float dragged_particle_initial_cam_z = 0.0f; // Store initial camera-space Z for dragging

    // 坐标系平移相关变量
    bool panning = false;
    sf::Vector2i pan_start_mouse;
    Vector3f pan_start_cam;

    float wind_strength = 0.0f;
    bool wind_on = false;

    float gravity = GRAVITY_CONST;

    bool tear_mode = false;

    bool display_info_message = false; // 用于控制左下角信息显示

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
                    sf::Vector2i mousePos = mouse->position; // Use raw pixel coords
                    float minDistSq = 1e18f; // Use squared distance
                    const float thresholdSq = 30.0f * 30.0f; // Squared threshold (30 pixels)
                    Particle* nearest = nullptr;
                    for (auto& particle : particles) {
                        sf::Vector2f projectedPos = project(particle.position);
                        // Skip particles projected way off-screen (e.g., behind camera)
                        if (projectedPos.x < -1000 || projectedPos.y < -1000)
                            continue;

                        sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
                        // Calculate squared distance in 2D screen space
                        float distSq = (projectedPos.x - mousePosF.x) * (projectedPos.x - mousePosF.x) + (projectedPos.y - mousePosF.y) * (projectedPos.y - mousePosF.y);

                        if (distSq < minDistSq && distSq < thresholdSq) {
                            minDistSq = distSq;
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
                        if (nearest && !nearest->is_pinned) {
                            dragging = true;
                            dragged_particle = nearest;
                            // Calculate and store initial camera-space Z
                            Vector3f initial_cam_coords = world_to_camera(dragged_particle->position);
                            dragged_particle_initial_cam_z = initial_cam_coords.z;
                        }
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
                    sf::Vector2i mousePos = mouse->position; // Use raw pixel coords
                    float minDistSq = 1e18f; // Use squared distance
                    const float thresholdSq = 30.0f * 30.0f; // Squared threshold (30 pixels)
                    Particle* nearest = nullptr;
                    for (auto& particle : particles) {
                        sf::Vector2f projectedPos = project(particle.position);
                        // Skip particles projected way off-screen (e.g., behind camera)
                        if (projectedPos.x < -1000 || projectedPos.y < -1000)
                            continue;

                        sf::Vector2f mousePosF(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
                        // Calculate squared distance in 2D screen space
                        float distSq = (projectedPos.x - mousePosF.x) * (projectedPos.x - mousePosF.x) + (projectedPos.y - mousePosF.y) * (projectedPos.y - mousePosF.y);

                        if (distSq < minDistSq && distSq < thresholdSq) {
                            minDistSq = distSq;
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
            /* // 暂时注释掉平移功能
            if (event->is<sf::Event::MouseButtonReleased>()) {
                auto mouse = event->getIf<sf::Event::MouseButtonReleased>();
                if (mouse && mouse->button == sf::Mouse::Button::Middle) {
                    panning = false;
                }
            }
            */
            // 空格键、R键、+/-键、[ / ]键处理
            if (event->is<sf::Event::KeyPressed>()) {
                auto key = event->getIf<sf::Event::KeyPressed>();
                if (key) {
                    bool camera_updated = false; // 标记相机角度/距离是否变化
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
                    // 相机旋转控制 (轨道模式)
                    if (key->code == sf::Keyboard::Key::A) {
                        cam_yaw -= 0.05f;
                        camera_updated = true;
                    }
                    if (key->code == sf::Keyboard::Key::D) {
                        cam_yaw += 0.05f;
                        camera_updated = true;
                    }
                    if (key->code == sf::Keyboard::Key::W) {
                        cam_pitch += 0.05f; // 增加俯仰角使相机"向上"移动
                        camera_updated = true;
                    }
                    if (key->code == sf::Keyboard::Key::S) {
                        cam_pitch -= 0.05f; // 减小俯仰角使相机"向下"移动
                        camera_updated = true;
                    }

                    // 如果相机角度发生变化，更新相机位置
                    if (camera_updated) {
                        update_camera_position();
                    }
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
                    // I键切换信息显示
                    if (key->code == sf::Keyboard::Key::I) {
                        display_info_message = !display_info_message;
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
            // 鼠标滚轮控制相机前后移动 (调整距离)
            if (event->is<sf::Event::MouseWheelScrolled>()) {
                auto wheel = event->getIf<sf::Event::MouseWheelScrolled>();
                if (wheel) {
                    // 计算相机前向向量 (不再需要，直接调整距离)
                    /*
                    float cy = std::cos(cam_yaw), sy = std::sin(cam_yaw);
                    float cx = std::cos(cam_pitch), sx = std::sin(cam_pitch);
                    Vector3f forward(
                        sy * cx,
                        -sx,
                        cy * cx);
                    float zoomSpeed = 50.0f;
                    cam_pos = cam_pos + forward * (wheel->delta * zoomSpeed);
                    */
                    float zoomAmount = wheel->delta * 50.0f; // 调整缩放灵敏度
                    cam_distance -= zoomAmount; // 减小距离实现放大效果
                    // 限制距离范围
                    if (cam_distance < 50.0f)
                        cam_distance = 50.0f; // 防止距离过近
                    if (cam_distance > 5000.0f)
                        cam_distance = 5000.0f; // 防止距离过远
                    update_camera_position(); // 根据新距离重新计算相机位置
                }
            }
            // 其他事件
            InputHandler::handle_mouse_click(*event, particles, constraints);
        }

        // 拖拽时让粒子跟随鼠标 (使用反向投影)
        if (dragging && dragged_particle) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);

            // --- Reverse Projection Calculation ---
            float screen_x = static_cast<float>(mousePos.x);
            float screen_y = static_cast<float>(mousePos.y);

            // 1. Screen to Camera Space (using stored Z)
            float initial_cam_z = dragged_particle_initial_cam_z;
            // Prevent issues if initial Z was too close or behind camera
            if (initial_cam_z <= 0.1f)
                initial_cam_z = 0.1f;

            float cam_x = (screen_x - WIDTH / 2.f) * initial_cam_z / fov_factor;
            float cam_y = -(screen_y - HEIGHT / 2.f) * initial_cam_z / fov_factor; // Y needs to be flipped back
            Vector3f P_cam(cam_x, cam_y, initial_cam_z); // Point in Camera Space

            // 2. Camera Space to World Space (reverse LookAt)
            // Re-calculate camera axes based on current cam_pos
            Vector3f target(0.0f, 0.0f, 0.0f);
            Vector3f world_up(0.0f, 1.0f, 0.0f);
            Vector3f zaxis = (cam_pos - target).normalized();
            Vector3f xaxis = world_up.cross(zaxis).normalized();
            Vector3f yaxis = zaxis.cross(xaxis);

            // Transform P_cam to world space (Adjust multiplication order)
            Vector3f new_world_pos = cam_pos + xaxis * P_cam.x + yaxis * P_cam.y - zaxis * P_cam.z;

            // Update particle position
            dragged_particle->position = new_world_pos;
            // Since we moved the particle, also update previous_position to avoid velocity jump
            dragged_particle->previous_position = new_world_pos;
        }

        // apply gravity and update particles
        for (auto& particle : particles) {
            particle.apply_force(Vector3f(0, -gravity, 0));
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

        // --- 添加绘制网格的代码 ---
        float grid_size = 2000.0f; // 网格总大小 (Increased)
        float grid_spacing = 50.0f; // 网格线间距
        sf::Color grid_color(80, 80, 80); // 网格线颜色（深灰色）

        // 绘制平行于 Z 轴的线 (改变 x, y=0 固定)
        for (float x = -grid_size / 2.0f; x <= grid_size / 2.0f; x += grid_spacing) {
            Vector3f start(x, 0, -grid_size / 2.0f);
            Vector3f end(x, 0, grid_size / 2.0f);
            sf::Vertex line[] = {
                { project(start), grid_color },
                { project(end), grid_color },
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }
        // 绘制平行于 X 轴的线 (改变 z, y=0 固定)
        for (float z = -grid_size / 2.0f; z <= grid_size / 2.0f; z += grid_spacing) {
            Vector3f start(-grid_size / 2.0f, 0, z);
            Vector3f end(grid_size / 2.0f, 0, z);
            sf::Vertex line[] = {
                { project(start), grid_color },
                { project(end), grid_color },
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }
        // --- 网格绘制结束 ---

        // --- 添加绘制 YZ 平面 (X=0) 网格的代码 ---
        // 绘制平行于 Y 轴的线 (改变 z, x=0 固定)
        for (float z = -grid_size / 2.0f; z <= grid_size / 2.0f; z += grid_spacing) {
            Vector3f start(0, -grid_size / 2.0f, z);
            Vector3f end(0, grid_size / 2.0f, z);
            sf::Vertex line[] = {
                { project(start), grid_color },
                { project(end), grid_color },
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }
        // 绘制平行于 Z 轴的线 (改变 y, x=0 固定)
        for (float y = -grid_size / 2.0f; y <= grid_size / 2.0f; y += grid_spacing) {
            Vector3f start(0, y, -grid_size / 2.0f);
            Vector3f end(0, y, grid_size / 2.0f);
            sf::Vertex line[] = {
                { project(start), grid_color },
                { project(end), grid_color },
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }
        // --- YZ 网格绘制结束 ---

        // --- 添加绘制 XY 平面 (Z=0) 网格的代码 ---
        // 绘制平行于 X 轴的线 (改变 y, z=0 固定)
        for (float y = -grid_size / 2.0f; y <= grid_size / 2.0f; y += grid_spacing) {
            Vector3f start(-grid_size / 2.0f, y, 0);
            Vector3f end(grid_size / 2.0f, y, 0);
            sf::Vertex line[] = {
                { project(start), grid_color },
                { project(end), grid_color },
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }
        // 绘制平行于 Y 轴的线 (改变 x, z=0 固定)
        for (float x = -grid_size / 2.0f; x <= grid_size / 2.0f; x += grid_spacing) {
            Vector3f start(x, -grid_size / 2.0f, 0);
            Vector3f end(x, grid_size / 2.0f, 0);
            sf::Vertex line[] = {
                { project(start), grid_color },
                { project(end), grid_color },
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }
        // --- XY 网格绘制结束 ---

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
            std::stringstream ss;
            ss << "Points: " << particles.size() << "\nConstraints: " << constraints.size() << "\nFPS: " << fps << "\nTear Mode: " << (tear_mode ? "ON" : "OFF") << "\nWind Mode: " << (wind_on ? "ON" : "OFF");
            ss << "\nGrid: ";
            if (grid_type == GridType::Square)
                ss << "Square";
            else if (grid_type == GridType::Triangle)
                ss << "Triangle";
            else if (grid_type == GridType::Hexagon)
                ss << "Hexagon";
            sf::Text info(font, ss.str());
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
            // 绘制坐标轴（加粗版 - Fixed)
            sf::Vector2f origin_2d(80, 120); // Top-left corner screen position
            float axis_len = 50; // Screen length of the axis representation
            float thick = 4.0f; // Line thickness

            // Project origin and points along axes to find screen direction
            Vector3f world_origin_pos(0, 0, 0);
            Vector3f world_x_end(axis_len, 0, 0); // Use axis_len temporarily for direction finding
            Vector3f world_y_end(0, axis_len, 0);
            Vector3f world_z_end(0, 0, axis_len);

            sf::Vector2f projected_origin = project(world_origin_pos);
            sf::Vector2f projected_x_end = project(world_x_end);
            sf::Vector2f projected_y_end = project(world_y_end);
            sf::Vector2f projected_z_end = project(world_z_end);

            // Calculate normalized 2D direction vectors on screen
            sf::Vector2f dir_x = projected_x_end - projected_origin;
            sf::Vector2f dir_y = projected_y_end - projected_origin;
            sf::Vector2f dir_z = projected_z_end - projected_origin;

            // Normalize the directions (handle potential zero vectors if projection fails)
            float len_x = std::sqrt(dir_x.x * dir_x.x + dir_x.y * dir_x.y);
            if (len_x > 1e-3f)
                dir_x /= len_x;
            float len_y = std::sqrt(dir_y.x * dir_y.x + dir_y.y * dir_y.y);
            if (len_y > 1e-3f)
                dir_y /= len_y;
            float len_z = std::sqrt(dir_z.x * dir_z.x + dir_z.y * dir_z.y);
            if (len_z > 1e-3f)
                dir_z /= len_z;

            // Function to draw thick lines (moved definition outside for clarity, or keep lambda)
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

            // Draw axes using calculated 2D directions
            draw_thick_line(origin_2d, origin_2d + dir_x * axis_len, sf::Color::Red); // X
            draw_thick_line(origin_2d, origin_2d + dir_y * axis_len, sf::Color::Green); // Y
            draw_thick_line(origin_2d, origin_2d + dir_z * axis_len, sf::Color::Blue); // Z

            // 绘制左下角动态信息
            if (display_info_message) {
                std::stringstream ss;
                ss << "Cam Pos: (" << cam_pos.x << ", " << cam_pos.y << ", " << cam_pos.z << ")\n"
                   << "Cam Yaw: " << cam_yaw << "\n"
                   << "Cam Pitch: " << cam_pitch << "\n"
                   << "Grid: ";
                if (grid_type == GridType::Square)
                    ss << "Square";
                else if (grid_type == GridType::Triangle)
                    ss << "Triangle";
                else if (grid_type == GridType::Hexagon)
                    ss << "Hexagon";
                ss << "\nParticles: " << particles.size();
                ss << "\nConstraints: " << constraints.size();
                std::string dynamic_info_str = ss.str(); // 从 stringstream 获取最终字符串

                sf::Text bottomLeftInfo(font, dynamic_info_str);
                bottomLeftInfo.setFillColor(sf::Color::Yellow);
                bottomLeftInfo.setCharacterSize(18);

                // 重新计算行数以确定位置
                int line_count = 1; // 从1开始计数
                for (char ch : dynamic_info_str) {
                    if (ch == '\n') {
                        line_count++;
                    }
                }

                float text_height = line_count * bottomLeftInfo.getCharacterSize() * 1.15f; // Approximate height with line spacing

                bottomLeftInfo.setPosition(sf::Vector2f(20.f, HEIGHT - text_height - 20.f));
                window.draw(bottomLeftInfo);
            }
        }

        window.display();
    }
}