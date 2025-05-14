#include "event_handler.h"
#include "constants.h" // For key codes, potentially
#include <iostream>    // For save/load messages

EventHandler::EventHandler(SimulationManager &sim, Camera &cam,
                           sf::RenderWindow &win)
    : sim_manager_(sim), camera_(cam), window_(win), dragging_(false),
      dragged_particle_(nullptr), dragged_particle_initial_cam_z_(0.0f),
      display_info_message_(false) {}

Particle *EventHandler::findNearestParticle(const sf::Vector2i &mouse_pos,
                                            float current_win_width,
                                            float current_win_height,
                                            const Camera &camera,
                                            float threshold_sq) {
    float min_dist_sq = 1e18f;
    Particle *nearest = nullptr;
    for (auto &particle :
         sim_manager_
             .getParticlesNonConst()) { // Need non-const access if Particle
                                        // methods are non-const
        sf::Vector2f projected_pos = camera.projectToScreen(
            particle.position, current_win_width, current_win_height);
        if (projected_pos.x < -1000 || projected_pos.y < -1000)
            continue; // Skip off-screen/behind camera

        sf::Vector2f mouse_pos_f(static_cast<float>(mouse_pos.x),
                                 static_cast<float>(mouse_pos.y));
        float dist_sq = (projected_pos.x - mouse_pos_f.x) *
                            (projected_pos.x - mouse_pos_f.x) +
                        (projected_pos.y - mouse_pos_f.y) *
                            (projected_pos.y - mouse_pos_f.y);

        if (dist_sq < min_dist_sq && dist_sq < threshold_sq) {
            min_dist_sq = dist_sq;
            nearest = &particle;
        }
    }
    return nearest;
}

Vector3f EventHandler::screenToWorld(const sf::Vector2i &mouse_pos,
                                     float target_cam_z,
                                     float current_win_width,
                                     float current_win_height,
                                     const Camera &camera) {
    float screen_x_norm = static_cast<float>(mouse_pos.x);
    float screen_y_norm = static_cast<float>(mouse_pos.y);

    // 1. Screen to Camera Space (using stored Z)
    float cam_x = (screen_x_norm - current_win_width / 2.f) * target_cam_z /
                  camera.getFovFactor();
    float cam_y = -(screen_y_norm - current_win_height / 2.f) * target_cam_z /
                  camera.getFovFactor();        // Y needs to be flipped back
    Vector3f p_cam(cam_x, cam_y, target_cam_z); // Point in Camera Space

    // 2. Camera Space to World Space (reverse LookAt)
    Vector3f cam_pos_current = camera.getPosition();
    Vector3f target_look_at(0.0f, 0.0f,
                            0.0f); // Assuming camera looks at origin
    Vector3f world_up(0.0f, 1.0f, 0.0f);

    Vector3f z_axis = (cam_pos_current - target_look_at).normalized();
    Vector3f x_axis = world_up.cross(z_axis).normalized();
    Vector3f y_axis = z_axis.cross(x_axis);

    // Transform P_cam to world space
    Vector3f new_world_pos = cam_pos_current + x_axis * p_cam.x +
                             y_axis * p_cam.y - z_axis * p_cam.z;
    return new_world_pos;
}

void EventHandler::processEvent(const sf::Event &event, float current_win_width,
                                float current_win_height) {
    if (event.is<sf::Event::KeyPressed>()) {
        handleKeyPressed(*event.getIf<sf::Event::KeyPressed>());
    }
    if (event.is<sf::Event::KeyReleased>()) {
        auto key = event.getIf<sf::Event::KeyReleased>();
        if (key && key->code == sf::Keyboard::Key::X) {
            sim_manager_.setTearMode(false);
        }
    }
    if (event.is<sf::Event::MouseButtonPressed>()) {
        handleMouseButtonPressed(*event.getIf<sf::Event::MouseButtonPressed>(),
                                 current_win_width, current_win_height);
    }
    if (event.is<sf::Event::MouseButtonReleased>()) {
        handleMouseButtonReleased(
            *event.getIf<sf::Event::MouseButtonReleased>());
    }
    if (event.is<sf::Event::MouseWheelScrolled>()) {
        handleMouseWheelScrolled(*event.getIf<sf::Event::MouseWheelScrolled>());
    }
    \n // The old InputHandler::handle_mouse_click from main.cpp was very
       // generic
    // and its specific logic seems to be covered by direct event handling here
    // or was placeholder.
}

void EventHandler::handleKeyPressed(const sf::Event::KeyEvent &key_event) {
    bool camera_updated = false;
    switch (key_event.code) {
    case sf::Keyboard::Key::X:
        sim_manager_.setTearMode(true);
        break;
    case sf::Keyboard::Key::Space:
        sim_manager_.toggleWind();
        break;
    case sf::Keyboard::Key::R:
        sim_manager_.resetCloth();
        break;
    case sf::Keyboard::Key::Equal: // Plus key
        sim_manager_.setGravity(sim_manager_.getGravity() + 1.0f);
        break;
    case sf::Keyboard::Key::Hyphen: // Minus key
        sim_manager_.setGravity(sim_manager_.getGravity() - 1.0f);
        break;
    case sf::Keyboard::Key::LBracket:
        sim_manager_.adjustWindStrength(-10.0f);
        break;
    case sf::Keyboard::Key::RBracket:
        sim_manager_.adjustWindStrength(10.0f);
        break;
    case sf::Keyboard::Key::A:
        camera_.adjustYaw(-0.05f);
        camera_updated = true;
        break;
    case sf::Keyboard::Key::D:
        camera_.adjustYaw(0.05f);
        camera_updated = true;
        break;
    case sf::Keyboard::Key::W:
        camera_.adjustPitch(0.05f);
        camera_updated = true;
        break;
    case sf::Keyboard::Key::S:
        if (!sf::Keyboard::isKeyPressed(
                sf::Keyboard::LControl)) { // Avoid conflict with Ctrl+S
            camera_.adjustPitch(-0.05f);
            camera_updated = true;
        }
        break;
    case sf::Keyboard::Key::T:
        sim_manager_.setGridType(GridType::Triangle);
        break;
    case sf::Keyboard::Key::H:
        sim_manager_.setGridType(GridType::Hexagon);
        break;
    case sf::Keyboard::Key::Q:
        sim_manager_.setGridType(GridType::Square);
        break;
    case sf::Keyboard::Key::I:
        display_info_message_ = !display_info_message_;
        break;
    // Save/Load
    case sf::Keyboard::Key::S: // Note: KeyPressed, not KeyReleased for Ctrl+S
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) {
            sim_manager_.saveState("cloth_save.txt");
        }
        break;
    case sf::Keyboard::Key::L:
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) {
            sim_manager_.loadState("cloth_save.txt");
        }
        break;
    default:
        break;
    }
    if (camera_updated) {
        camera_.updatePosition();
    }
}

void EventHandler::handleMouseButtonPressed(
    const sf::Event::MouseButtonEvent &mouse_event, float current_win_width,
    float current_win_height) {
    if (mouse_event.button == sf::Mouse::Left) {
        Particle *nearest =
            findNearestParticle(mouse_event.position, current_win_width,
                                current_win_height, camera_);
        if (sim_manager_.isTearMode() && nearest) {
            sim_manager_.handleParticleTear(nearest);
        } else {
            if (nearest && !nearest->is_pinned) {
                dragging_ = true;
                dragged_particle_ = nearest;
                Vector3f initial_cam_coords = camera_.worldToCameraCoordinates(
                    dragged_particle_->position);
                dragged_particle_initial_cam_z_ = initial_cam_coords.z;
                if (dragged_particle_initial_cam_z_ <= 0.1f)
                    dragged_particle_initial_cam_z_ =
                        0.1f; // Ensure positive depth
            }
        }
    } else if (mouse_event.button == sf::Mouse::Right) {
        Particle *nearest =
            findNearestParticle(mouse_event.position, current_win_width,
                                current_win_height, camera_);
        if (nearest) {
            nearest->is_pinned = !nearest->is_pinned;
        }
    } else if (mouse_event.button == sf::Mouse::Middle) {
        // Panning logic would go here if re-enabled
        // panning_ = true;
        // pan_start_mouse_ = mouse_event.position;
        // pan_start_cam_pos_ = camera_.getPosition();
    }
}

void EventHandler::handleMouseButtonReleased(
    const sf::Event::MouseButtonEvent &mouse_event) {
    if (mouse_event.button == sf::Mouse::Left) {
        dragging_ = false;
        dragged_particle_ = nullptr;
    }
    // if (mouse_event.button == sf::Mouse::Middle) {
    //     panning_ = false;
    // }
}

void EventHandler::handleMouseWheelScrolled(
    const sf::Event::MouseWheelScrollEvent &wheel_event) {
    float zoomAmount = wheel_event.delta * 50.0f;
    camera_.adjustDistance(zoomAmount); // adjustDistance in camera now expects
                                        // positive for zoom out
    camera_.updatePosition();
}

void EventHandler::updateMouseDrag(float current_win_width,
                                   float current_win_height,
                                   const Camera &camera) {
    if (dragging_ && dragged_particle_) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(
            window_); // Use the window reference passed in constructor
        Vector3f new_world_pos =
            screenToWorld(mousePos, dragged_particle_initial_cam_z_,
                          current_win_width, current_win_height, camera);

        dragged_particle_->position = new_world_pos;
        dragged_particle_->previous_position =
            new_world_pos; // Avoid velocity jump
    }
}