#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <SFML/Window.hpp>
#include <SFML/Graphics/RenderWindow.hpp> // For sf::Mouse::getPosition
#include "simulation_manager.h"
#include "camera.h"
// #include "renderer.h" // Might need for toggling display info if Renderer
// holds that state

class EventHandler {
  public:
    EventHandler(SimulationManager &sim, Camera &cam, sf::RenderWindow &win);

    void processEvent(const sf::Event &event, float current_win_width,
                      float current_win_height);
    void updateMouseDrag(float current_win_width, float current_win_height,
                         const Camera &camera); // Pass camera for projection

    bool shouldDisplayInfo() const {
        return display_info_message_;
    }
    void toggleDisplayInfo() {
        display_info_message_ = !display_info_message_;
    }

  private:
    SimulationManager &sim_manager_;
    Camera &camera_;
    sf::RenderWindow &window_; // Needed for mouse position relative to window

    // Interaction states
    bool dragging_;
    Particle *dragged_particle_;
    float dragged_particle_initial_cam_z_;

    // Panning state (currently disabled in main.cpp, but kept for structure)
    // bool panning_;
    // sf::Vector2i pan_start_mouse_;
    // Vector3f pan_start_cam_pos_;

    bool display_info_message_; // For toggling dynamic info display

    void handleKeyPressed(const sf::Event::KeyEvent &key_event);
    void
    handleMouseButtonPressed(const sf::Event::MouseButtonEvent &mouse_event,
                             float current_win_width, float current_win_height);
    void
    handleMouseButtonReleased(const sf::Event::MouseButtonEvent &mouse_event);
    void handleMouseWheelScrolled(
        const sf::Event::MouseWheelScrollEvent &wheel_event);

    Particle *findNearestParticle(const sf::Vector2i &mouse_pos,
                                  float current_win_width,
                                  float current_win_height,
                                  const Camera &camera,
                                  float threshold_sq = (30.0f * 30.0f));
    Vector3f screenToWorld(const sf::Vector2i &mouse_pos, float target_cam_z,
                           float current_win_width, float current_win_height,
                           const Camera &camera);
};

#endif // EVENT_HANDLER_H