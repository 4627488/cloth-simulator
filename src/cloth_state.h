#pragma once
#include "constraint.h"
#include "particle.h"
#include <string>
#include <vector>

class ClothState {
public:
    static bool save(const std::vector<Particle>& particles, const std::vector<Constraint>& constraints, const std::string& filename);
    static bool load(std::vector<Particle>& particles, std::vector<Constraint>& constraints, const std::string& filename);
};