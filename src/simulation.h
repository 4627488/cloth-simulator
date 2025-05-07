#pragma once
#include "cloth.h"
#include <SFML/Graphics.hpp>

class Simulation {
public:
    Simulation();
    void run();

private:
    sf::RenderWindow window;
    Cloth cloth;
    float gravity;
    float wind_strength;
    bool wind_on;
    int satisfy_iter;
    void handle_events();
    void draw_ui();
    void process_mouse(const sf::Event& event);
    void process_keyboard(const sf::Event& event);
};