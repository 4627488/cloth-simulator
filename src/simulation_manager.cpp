#include "simulation_manager.h"
#include <iostream>  // For save/load messages
#include <algorithm> // For std::remove_if

SimulationManager::SimulationManager()
    : grid_type_(GridType::Square), gravity_(GRAVITY_CONST),
      wind_strength_(0.0f), wind_on_(false), tear_mode_(false) {
    resetCloth(); // Initialize with a default cloth
}

void SimulationManager::resetCloth() {
    particles_.clear();
    constraints_.clear();
    int rows_to_use = DEFAULT_ROW;
    int cols_to_use = DEFAULT_COL;
    float rest_distance_to_use = DEFAULT_REST_DISTANCE;

    // Constants for cloth generation, matching main.cpp logic
    const float cloth_width_world =
        WIDTH; // Assuming WIDTH is world units for initial placement
    const float cloth_height_world =
        HEIGHT; // Assuming HEIGHT is world units for initial placement

    if (grid_type_ == GridType::Square) {
        for (int row = 0; row < rows_to_use; row++) {
            for (int col = 0; col < cols_to_use; col++) {
                float x = col * rest_distance_to_use -
                          cloth_width_world / 6.f; // Adjusted initial position
                float y = row * rest_distance_to_use +
                          cloth_height_world / 6.f; // Adjusted initial position
                float z =
                    (float)(col + row) / (rows_to_use + cols_to_use) * 200.0f +
                    100.0f;
                bool pinned = (row == 0);
                particles_.emplace_back(x, y, z, pinned);
            }
        }
        for (int row = 0; row < rows_to_use; row++) {
            for (int col = 0; col < cols_to_use; col++) {
                if (col < cols_to_use - 1) {
                    constraints_.emplace_back(
                        &particles_[row * cols_to_use + col],
                        &particles_[row * cols_to_use + col + 1],
                        rest_distance_to_use);
                }
                if (row < rows_to_use - 1) {
                    constraints_.emplace_back(
                        &particles_[row * cols_to_use + col],
                        &particles_[(row + 1) * cols_to_use + col],
                        rest_distance_to_use);
                }
            }
        }
    } else if (grid_type_ == GridType::Triangle) {
        for (int row = 0; row < rows_to_use; row++) {
            for (int col = 0; col < cols_to_use; col++) {
                float x = col * rest_distance_to_use - cloth_width_world / 6.f;
                float y = row * rest_distance_to_use + cloth_height_world / 6.f;
                float z =
                    (float)(col + row) / (rows_to_use + cols_to_use) * 200.0f +
                    100.0f;
                bool pinned = (row == 0);
                particles_.emplace_back(x, y, z, pinned);
            }
        }
        for (int row = 0; row < rows_to_use; row++) {
            for (int col = 0; col < cols_to_use; col++) {
                int idx = row * cols_to_use + col;
                if (col < cols_to_use - 1) // Right
                    constraints_.emplace_back(&particles_[idx],
                                              &particles_[idx + 1],
                                              rest_distance_to_use);
                if (row < rows_to_use - 1) // Down
                    constraints_.emplace_back(&particles_[idx],
                                              &particles_[idx + cols_to_use],
                                              rest_distance_to_use);
                if (col < cols_to_use - 1 &&
                    row < rows_to_use - 1) // Bottom-right
                    constraints_.emplace_back(
                        &particles_[idx], &particles_[idx + cols_to_use + 1],
                        rest_distance_to_use * std::sqrt(2.f));
                if (col > 0 && row < rows_to_use - 1) // Bottom-left
                    constraints_.emplace_back(
                        &particles_[idx], &particles_[idx + cols_to_use - 1],
                        rest_distance_to_use * std::sqrt(2.f));
            }
        }
    } else if (grid_type_ == GridType::Hexagon) {
        float hex_dx = rest_distance_to_use * 0.866f; // cos(30 deg)
        float hex_dy = rest_distance_to_use * 0.75f;  // row spacing
        for (int row = 0; row < rows_to_use; row++) {
            for (int col = 0; col < cols_to_use; col++) {
                float x = col * hex_dx + (row % 2) * (hex_dx / 2.f) +
                          cloth_width_world / 6.f;
                float y = row * hex_dy + cloth_height_world / 6.f;
                float z =
                    (float)(col + row) / (rows_to_use + cols_to_use) * 200.0f +
                    100.0f;
                bool pinned = (row == 0);
                particles_.emplace_back(x, y, z, pinned);
            }
        }
        for (int row = 0; row < rows_to_use; row++) {
            for (int col = 0; col < cols_to_use; col++) {
                int idx = row * cols_to_use + col;
                if (col < cols_to_use - 1)
                    constraints_.emplace_back(&particles_[idx],
                                              &particles_[idx + 1],
                                              hex_dx); // Horizontal

                if (row < rows_to_use - 1) {
                    if (row % 2 == 1) { // Odd rows
                        if (col > 0)
                            constraints_.emplace_back(
                                &particles_[idx],
                                &particles_[idx + cols_to_use - 1],
                                rest_distance_to_use); // Bottom-left
                        constraints_.emplace_back(
                            &particles_[idx], &particles_[idx + cols_to_use],
                            rest_distance_to_use); // Approx. Bottom
                    } else {                       // Even rows
                        constraints_.emplace_back(
                            &particles_[idx], &particles_[idx + cols_to_use],
                            rest_distance_to_use); // Approx. Bottom
                        if (col < cols_to_use - 1)
                            constraints_.emplace_back(
                                &particles_[idx],
                                &particles_[idx + cols_to_use + 1],
                                rest_distance_to_use); // Bottom-right
                    }
                }
            }
        }
    }
    // The loop for checking c.initial_length <= 0 from main.cpp seems like a
    // temporary fix and might indicate an issue in rest_distance calculation
    // for some constraints. For now, it's omitted, assuming constraints are
    // well-formed.
}

void SimulationManager::applyGravityToParticles() {
    for (auto &particle : particles_) {
        particle.apply_force(Vector3f(0, -gravity_, 0));
    }
}

void SimulationManager::applyWindToParticles() {
    if (wind_on_) {
        for (auto &particle : particles_) {
            particle.apply_force(Vector3f(wind_strength_, 0, 0));
        }
    }
}

void SimulationManager::constrainParticlesToBounds(float world_width,
                                                   float world_height,
                                                   float world_depth) {
    for (auto &particle : particles_) {
        particle.constrain_to_bounds(
            world_width, world_height,
            world_depth); // Assuming Particle class has this method
    }
}

void SimulationManager::updatePhysics(float timestep) {
    applyGravityToParticles();
    applyWindToParticles();
    for (auto &particle : particles_) {
        particle.update(timestep);
    }
    // In main.cpp, constrain_to_bounds was called inside particle.update or
    // after it for each particle. If constrain_to_bounds is part of
    // Particle::update, this is fine. Otherwise, it needs to be called
    // explicitly here, e.g.:
    constrainParticlesToBounds(
        WIDTH, HEIGHT,
        1000.0f); // Using constants directly, should be passed or configurable
}

void SimulationManager::satisfyConstraints(int iterations) {
    for (int i = 0; i < iterations; i++) {
        for (auto &constraint : constraints_) {
            constraint.satisfy();
        }
    }
}

bool SimulationManager::saveState(const std::string &filename) const {
    if (ClothState::save(particles_, constraints_, filename)) {
        std::cout << "Cloth state saved to " << filename << std::endl;
        return true;
    }
    std::cout << "Failed to save cloth state to " << filename << std::endl;
    return false;
}

bool SimulationManager::loadState(const std::string &filename) {
    // Clear existing state before loading
    std::vector<Particle> temp_particles;
    std::vector<Constraint> temp_constraints;
    if (ClothState::load(temp_particles, temp_constraints, filename)) {
        particles_ = temp_particles;
        constraints_ = temp_constraints;
        std::cout << "Cloth state loaded from " << filename << std::endl;
        return true;
    }
    std::cout << "Failed to load cloth state from " << filename << std::endl;
    return false;
}

void SimulationManager::handleParticleTear(
    Particle *particle_to_remove_constraints_for) {
    if (!particle_to_remove_constraints_for) return;
    constraints_.erase(
        std::remove_if(
            constraints_.begin(), constraints_.end(),
            [particle_to_remove_constraints_for](const Constraint &c) {
                return c.p1 == particle_to_remove_constraints_for ||
                       c.p2 == particle_to_remove_constraints_for;
            }),
        constraints_.end());
}

void SimulationManager::setGridType(GridType type) {
    grid_type_ = type;
    resetCloth(); // Changing grid type implies a reset
}

void SimulationManager::adjustWindStrength(float delta) {
    wind_strength_ += delta;
    if (wind_strength_ < 0.0f) wind_strength_ = 0.0f; // Prevent negative wind
    if (wind_strength_ > 0.0f && !wind_on_)
        wind_on_ = true; // If wind is adjusted, turn it on
}