#include "renderer.h"
#include "constants.h" // For PARTICLE_RADIUS, HEIGHT etc.
#include <iostream>    // For font loading error

Renderer::Renderer(sf::RenderWindow &window)
    : window_(window), font_loaded_(false) {
    font_loaded_ = loadFont();
}

std::vector<std::string> Renderer::getFontPaths() const {
    std::vector<std::string> paths;
    paths.push_back("Arial.ttf");
    paths.push_back("fonts/Arial.ttf");
#if defined(_WIN32)
    paths.push_back("C:/Windows/Fonts/Arial.ttf");
#elif defined(__APPLE__)
    paths.push_back("/System/Library/Fonts/Supplemental/Arial.ttf");
#elif defined(__linux__)
    paths.push_back(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"); // A common Linux
                                                            // fallback
#endif
    return paths;
}

bool Renderer::loadFont() {
    auto paths = getFontPaths();
    for (const auto &path : paths) {
        if (font_.openFromFile(path)) {
            return true;
        }
    }
    std::cerr << "Font loading failed. Please place a valid font (e.g., "
                 "Arial.ttf) in the program directory, or fonts/ subdirectory, "
                 "or ensure system fonts are accessible."
              << std::endl;
    return false;
}

void Renderer::clear() {
    window_.clear(sf::Color::Black);
}

void Renderer::display() {
    window_.display();
}

void Renderer::drawGridLines(const Camera &camera, float current_win_width,
                             float current_win_height) {
    float grid_size = 2000.0f;
    float grid_spacing = 50.0f;
    sf::Color grid_color(80, 80, 80);

    // XZ plane (y=0)
    for (float i = -grid_size / 2.0f; i <= grid_size / 2.0f;
         i += grid_spacing) {
        Vector3f start_xz1(i, 0, -grid_size / 2.0f);
        Vector3f end_xz1(i, 0, grid_size / 2.0f);
        sf::Vertex line_xz1[] = {
            {camera.projectToScreen(start_xz1, current_win_width,
                                    current_win_height),
             grid_color},
            {camera.projectToScreen(end_xz1, current_win_width,
                                    current_win_height),
             grid_color},
        };
        window_.draw(line_xz1, 2, sf::PrimitiveType::Lines);

        Vector3f start_xz2(-grid_size / 2.0f, 0, i);
        Vector3f end_xz2(grid_size / 2.0f, 0, i);
        sf::Vertex line_xz2[] = {
            {camera.projectToScreen(start_xz2, current_win_width,
                                    current_win_height),
             grid_color},
            {camera.projectToScreen(end_xz2, current_win_width,
                                    current_win_height),
             grid_color},
        };
        window_.draw(line_xz2, 2, sf::PrimitiveType::Lines);
    }
    // YZ plane (x=0)
    for (float i = -grid_size / 2.0f; i <= grid_size / 2.0f;
         i += grid_spacing) {
        Vector3f start_yz1(0, i, -grid_size / 2.0f);
        Vector3f end_yz1(0, i, grid_size / 2.0f);
        sf::Vertex line_yz1[] = {
            {camera.projectToScreen(start_yz1, current_win_width,
                                    current_win_height),
             grid_color},
            {camera.projectToScreen(end_yz1, current_win_width,
                                    current_win_height),
             grid_color},
        };
        window_.draw(line_yz1, 2, sf::PrimitiveType::Lines);

        Vector3f start_yz2(0, -grid_size / 2.0f, i);
        Vector3f end_yz2(0, grid_size / 2.0f, i);
        sf::Vertex line_yz2[] = {
            {camera.projectToScreen(start_yz2, current_win_width,
                                    current_win_height),
             grid_color},
            {camera.projectToScreen(end_yz2, current_win_width,
                                    current_win_height),
             grid_color},
        };
        window_.draw(line_yz2, 2, sf::PrimitiveType::Lines);
    }
    // XY plane (z=0)
    for (float i = -grid_size / 2.0f; i <= grid_size / 2.0f;
         i += grid_spacing) {
        Vector3f start_xy1(i, -grid_size / 2.0f, 0);
        Vector3f end_xy1(i, grid_size / 2.0f, 0);
        sf::Vertex line_xy1[] = {
            {camera.projectToScreen(start_xy1, current_win_width,
                                    current_win_height),
             grid_color},
            {camera.projectToScreen(end_xy1, current_win_width,
                                    current_win_height),
             grid_color},
        };
        window_.draw(line_xy1, 2, sf::PrimitiveType::Lines);

        Vector3f start_xy2(-grid_size / 2.0f, i, 0);
        Vector3f end_xy2(grid_size / 2.0f, i, 0);
        sf::Vertex line_xy2[] = {
            {camera.projectToScreen(start_xy2, current_win_width,
                                    current_win_height),
             grid_color},
            {camera.projectToScreen(end_xy2, current_win_width,
                                    current_win_height),
             grid_color},
        };
        window_.draw(line_xy2, 2, sf::PrimitiveType::Lines);
    }
}

void Renderer::drawParticles(const std::vector<Particle> &particles,
                             const Camera &camera, float current_win_width,
                             float current_win_height) {
    for (const auto &particle : particles) {
        sf::Color color =
            particle.is_pinned
                ? sf::Color::Red
                : sf::Color(255,
                            255 - (int)(particle.position.y / HEIGHT * 255),
                            255 - (int)(particle.position.z / 1000.0f * 255));
        sf::Vertex point = {camera.projectToScreen(particle.position,
                                                   current_win_width,
                                                   current_win_height),
                            color};
        window_.draw(&point, 1, sf::PrimitiveType::Points);
    }
}

void Renderer::drawConstraints(const std::vector<Constraint> &constraints,
                               const Camera &camera, float current_win_width,
                               float current_win_height) {
    for (const auto &constraint : constraints) {
        if (!constraint.active || !constraint.p1 || !constraint.p2) {
            continue;
        }
        float len =
            (constraint.p1->position - constraint.p2->position).length();
        float t = std::min(std::abs(len - constraint.initial_length) /
                               (constraint.initial_length * 0.5f),
                           1.0f);
        sf::Color lineColor = sf::Color(255, (sf::Uint8)(255 * (1 - t)),
                                        (sf::Uint8)(255 * (1 - t)));
        sf::Vertex line[] = {
            {camera.projectToScreen(constraint.p1->position, current_win_width,
                                    current_win_height),
             lineColor},
            {camera.projectToScreen(constraint.p2->position, current_win_width,
                                    current_win_height),
             lineColor},
        };
        window_.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

void Renderer::drawStatsPanel(const SimulationManager &sim_manager, float fps,
                              bool tear_mode, bool wind_on,
                              float current_win_width) {
    if (!font_loaded_) return;
    std::stringstream ss;
    ss.precision(1);
    ss << std::fixed;
    ss << "Points: " << sim_manager.getParticles().size()
       << "\nConstraints: " << sim_manager.getConstraints().size()
       << "\nFPS: " << fps << "\nTear Mode: " << (tear_mode ? "ON" : "OFF")
       << "\nWind Mode: " << (wind_on ? "ON" : "OFF");
    ss << "\nGrid: ";
    GridType grid_type = sim_manager.getGridType();
    if (grid_type == GridType::Square)
        ss << "Square";
    else if (grid_type == GridType::Triangle)
        ss << "Triangle";
    else if (grid_type == GridType::Hexagon)
        ss << "Hexagon";

    sf::Text info_text(ss.str(), font_);
    info_text.setFillColor(sf::Color::White);
    info_text.setPosition(
        sf::Vector2f(static_cast<float>(current_win_width - 350), 20.f));
    window_.draw(info_text);
}

void Renderer::drawHelpPanel(float current_win_width,
                             float current_win_height) {
    if (!font_loaded_) return;
    std::string help_str = "Left click: Drag particle\n"
                           "Right click: Pin/unpin\n"
                           "Middle click: Pan (Disabled)\n"
                           "Mouse wheel: Zoom\n"
                           "R: Reset cloth\n"
                           "+/-: Gravity\n"
                           "[/]: Wind Strength\n"
                           "WASD: Rotate view\n"
                           "Space: Toggle wind\n"
                           "T: Triangle grid\n"
                           "H: Hex grid\n"
                           "Q: Square grid\n"
                           "Ctrl+S: Save cloth\n"
                           "Ctrl+L: Load cloth\n"
                           "I: Toggle Info";

    sf::Text help_text(help_str, font_);
    help_text.setFillColor(sf::Color(200, 200, 200));
    help_text.setCharacterSize(22);
    auto help_bounds = help_text.getLocalBounds();
    help_text.setPosition(
        sf::Vector2f(current_win_width - help_bounds.width - 40.f,
                     current_win_height - help_bounds.height - 80.f));
    window_.draw(help_text);
}

void Renderer::drawThickLine(const sf::Vector2f &from, const sf::Vector2f &to,
                             sf::Color color, float thickness) {
    sf::Vector2f dir = to - from;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 1e-3f) return;
    dir /= len;
    sf::Vector2f normal(-dir.y, dir.x);

    sf::Vector2f p0 = from + normal * (thickness / 2.f);
    sf::Vector2f p1 = from - normal * (thickness / 2.f);
    sf::Vector2f p2 = to - normal * (thickness / 2.f);
    sf::Vector2f p3 = to + normal * (thickness / 2.f);

    sf::Vertex tri[6] = {{p0, color}, {p1, color}, {p2, color},
                         {p0, color}, {p2, color}, {p3, color}};
    window_.draw(tri, 6, sf::PrimitiveType::Triangles);
}

void Renderer::drawCoordinateAxes(const Camera &camera, float current_win_width,
                                  float current_win_height) {
    if (!font_loaded_)
        return; // Optional: Axes labels could also depend on font

    sf::Vector2f origin_2d(80.f, 120.f); // Screen position for the axis widget
    float axis_screen_len = 50.f;
    float thickness = 4.0f;

    Vector3f world_origin_pos(0, 0, 0);
    // Use a small offset along each axis to determine projected direction
    float dir_scale = 10.0f; // Small world unit length for direction projection
    Vector3f world_x_end(dir_scale, 0, 0);
    Vector3f world_y_end(0, dir_scale, 0);
    Vector3f world_z_end(0, 0, dir_scale);

    sf::Vector2f projected_origin = camera.projectToScreen(
        world_origin_pos, current_win_width, current_win_height);
    sf::Vector2f projected_x_end = camera.projectToScreen(
        world_x_end, current_win_width, current_win_height);
    sf::Vector2f projected_y_end = camera.projectToScreen(
        world_y_end, current_win_width, current_win_height);
    sf::Vector2f projected_z_end = camera.projectToScreen(
        world_z_end, current_win_width, current_win_height);

    auto calculate_direction = [&](const sf::Vector2f &start,
                                   const sf::Vector2f &end) {
        sf::Vector2f dir = end - start;
        float l = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        return (l > 1e-3f)
                   ? (dir / l)
                   : sf::Vector2f(0, 0); // Return normalized or zero vector
    };

    sf::Vector2f dir_x = calculate_direction(projected_origin, projected_x_end);
    sf::Vector2f dir_y = calculate_direction(projected_origin, projected_y_end);
    sf::Vector2f dir_z = calculate_direction(projected_origin, projected_z_end);

    // If a projected direction is zero (e.g. axis points directly at camera),
    // pick a default fallback to ensure something is drawn. This is a
    // simplified fallback.
    if (dir_x.x == 0 && dir_x.y == 0)
        dir_x =
            sf::Vector2f(1, 0); // Default to screen right for X if degenerate
    if (dir_y.x == 0 && dir_y.y == 0)
        dir_y = sf::Vector2f(
            0,
            -1); // Default to screen up for Y if degenerate (screen Y is down)
    if (dir_z.x == 0 && dir_z.y == 0)
        dir_z = sf::Vector2f(0.707f, 0.707f); // Default to some diagonal for Z

    drawThickLine(origin_2d, origin_2d + dir_x * axis_screen_len,
                  sf::Color::Red, thickness);
    drawThickLine(origin_2d, origin_2d + dir_y * axis_screen_len,
                  sf::Color::Green, thickness);
    drawThickLine(origin_2d, origin_2d + dir_z * axis_screen_len,
                  sf::Color::Blue, thickness);

    // Labels (Optional, simple example)
    sf::Text x_label("X", font_, 15);
    x_label.setFillColor(sf::Color::Red);
    x_label.setPosition(origin_2d + dir_x * axis_screen_len * 1.1f);
    window_.draw(x_label);

    sf::Text y_label("Y", font_, 15);
    y_label.setFillColor(sf::Color::Green);
    y_label.setPosition(origin_2d + dir_y * axis_screen_len * 1.1f);
    window_.draw(y_label);

    sf::Text z_label("Z", font_, 15);
    z_label.setFillColor(sf::Color::Blue);
    z_label.setPosition(origin_2d + dir_z * axis_screen_len * 1.1f);
    window_.draw(z_label);
}

void Renderer::drawDynamicInfoPanel(const Camera &camera,
                                    const SimulationManager &sim_manager,
                                    bool display_info, float current_win_width,
                                    float current_win_height) {
    if (!font_loaded_ || !display_info) return;

    std::stringstream ss;
    ss.precision(1);
    ss << std::fixed;
    ss << "Cam Pos: (" << camera.getPosition().x << ", "
       << camera.getPosition().y << ", " << camera.getPosition().z << ")\n"
       << "Cam Yaw: " << camera.getYaw() << "\n"
       << "Cam Pitch: " << camera.getPitch() << "\n"
       << "Grid: ";
    GridType grid_type = sim_manager.getGridType();
    if (grid_type == GridType::Square)
        ss << "Square";
    else if (grid_type == GridType::Triangle)
        ss << "Triangle";
    else if (grid_type == GridType::Hexagon)
        ss << "Hexagon";
    ss << "\nParticles: " << sim_manager.getParticles().size();
    ss << "\nConstraints: " << sim_manager.getConstraints().size();

    std::string dynamic_info_str = ss.str();
    sf::Text bottom_left_info(dynamic_info_str, font_);
    bottom_left_info.setFillColor(sf::Color::Yellow);
    bottom_left_info.setCharacterSize(18);

    int line_count = 1;
    for (char ch : dynamic_info_str) {
        if (ch == '\n') line_count++;
    }
    float text_height =
        line_count * bottom_left_info.getCharacterSize() * 1.15f;

    bottom_left_info.setPosition(
        sf::Vector2f(20.f, current_win_height - text_height - 20.f));
    window_.draw(bottom_left_info);
}