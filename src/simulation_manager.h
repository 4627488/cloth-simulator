#ifndef SIMULATION_MANAGER_H
#define SIMULATION_MANAGER_H

#include <vector>
#include <string>
#include <cmath> // For std::sqrt
#include "particle.h"
#include "constraint.h"
#include "vector3f.h"
#include "constants.h"   // For DEFAULT_ROW, DEFAULT_COL, etc.
#include "cloth_state.h" // For save/load functionality

// GridType enum, moved from main.cpp
enum class GridType { Square, Triangle, Hexagon };

class SimulationManager {
  public:
    SimulationManager();

    void resetCloth();
    void updatePhysics(float timestep);
    void satisfyConstraints(int iterations = 5);
    void applyGravityToParticles();
    void applyWindToParticles();
    void constrainParticlesToBounds(float world_width, float world_height,
                                    float world_depth);

    bool saveState(const std::string &filename) const;
    bool loadState(const std::string &filename);

    void handleParticleTear(Particle *particle_to_remove_constraints_for);

    // Getters
    const std::vector<Particle> &getParticles() const {
        return particles_;
    }
    const std::vector<Constraint> &getConstraints() const {
        return constraints_;
    }
    GridType getGridType() const {
        return grid_type_;
    }
    float getGravity() const {
        return gravity_;
    }
    float getWindStrength() const {
        return wind_strength_;
    }
    bool isWindOn() const {
        return wind_on_;
    }
    bool isTearMode() const {
        return tear_mode_;
    }
    std::vector<Particle> &getParticlesNonConst() {
        return particles_;
    } // For dragging

    // Setters
    void setGridType(GridType type); // Will also call resetCloth()
    void setGravity(float gravity) {
        gravity_ = gravity;
    }
    void setWindStrength(float strength) {
        wind_strength_ = strength;
    }
    void toggleWind() {
        wind_on_ = !wind_on_;
        if (!wind_on_)
            wind_strength_ = 0.0f;
        else if (wind_strength_ == 0.0f)
            wind_strength_ = 100.0f;
    }
    void setWindOn(bool on) {
        wind_on_ = on;
        if (!wind_on_) wind_strength_ = 0.0f;
    }
    void adjustWindStrength(float delta);
    void toggleTearMode() {
        tear_mode_ = !tear_mode_;
    }
    void setTearMode(bool mode) {
        tear_mode_ = mode;
    }

  private:
    std::vector<Particle> particles_;
    std::vector<Constraint> constraints_;
    GridType grid_type_;
    float gravity_;
    float wind_strength_;
    bool wind_on_;
    bool tear_mode_;
};

#endif // SIMULATION_MANAGER_H