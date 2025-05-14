#ifndef RENDERER_H
#define RENDERER_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <sstream> // For string stream in UI text
#include "camera.h"
#include "simulation_manager.h" // For particle, constraint, grid_type data
#include "particle.h"           // For Particle data
#include "constraint.h"         // For Constraint data

class Renderer {
  public:
    Renderer(sf::RenderWindow &window);

    bool loadFont(); // Tries multiple paths

    void clear();
    void display();

    void drawGridLines(const Camera &camera, float current_win_width,
                       float current_win_height);
    void drawParticles(const std::vector<Particle> &particles,
                       const Camera &camera, float current_win_width,
                       float current_win_height);
    void drawConstraints(const std::vector<Constraint> &constraints,
                         const Camera &camera, float current_win_width,
                         float current_win_height);

    void drawStatsPanel(const SimulationManager &sim_manager, float fps,
                        bool tear_mode, bool wind_on, float current_win_width);
    void drawHelpPanel(float current_win_width, float current_win_height);
    void drawCoordinateAxes(const Camera &camera, float current_win_width,
                            float current_win_height);
    void drawDynamicInfoPanel(const Camera &camera,
                              const SimulationManager &sim_manager,
                              bool display_info, float current_win_width,
                              float current_win_height);

  private:
    sf::RenderWindow &window_;
    sf::Font font_;
    bool font_loaded_;

    void drawThickLine(const sf::Vector2f &from, const sf::Vector2f &to,
                       sf::Color color, float thickness);
    std::vector<std::string> getFontPaths() const;
};

#endif // RENDERER_H