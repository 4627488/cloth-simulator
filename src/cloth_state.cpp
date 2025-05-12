#include "cloth_state.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

bool ClothState::save(const std::vector<Particle>& particles, const std::vector<Constraint>& constraints, const std::string& filename)
{
    std::ofstream ofs(filename);
    if (!ofs)
        return false;
    ofs << "# particles\n";
    for (const auto& p : particles) {
        ofs << p.position.x << " " << p.position.y << " " << p.position.z << " " << p.is_pinned << "\n";
    }
    ofs << "# constraints\n";
    for (const auto& c : constraints) {
        int idx1 = static_cast<int>(c.p1 - &particles[0]);
        int idx2 = static_cast<int>(c.p2 - &particles[0]);
        ofs << idx1 << " " << idx2 << "\n";
    }
    return true;
}

bool ClothState::load(std::vector<Particle>& particles, std::vector<Constraint>& constraints, const std::string& filename)
{
    std::ifstream ifs(filename);
    if (!ifs)
        return false;
    std::string line;
    particles.clear();
    constraints.clear();
    // 读取粒子
    while (std::getline(ifs, line)) {
        if (line.find("# particles") != std::string::npos)
            continue;
        if (line.find("# constraints") != std::string::npos)
            break;
        if (line.empty())
            continue;
        float x, y, z;
        int pinned;
        std::istringstream iss(line);
        if (!(iss >> x >> y >> z >> pinned))
            continue;
        particles.emplace_back(x, y, z, pinned != 0);
    }
    // 读取约束
    while (std::getline(ifs, line)) {
        if (line.empty())
            continue;
        int idx1, idx2;
        std::istringstream iss(line);
        if (!(iss >> idx1 >> idx2))
            continue;
        if (idx1 >= 0 && idx1 < particles.size() && idx2 >= 0 && idx2 < particles.size())
            constraints.emplace_back(&particles[idx1], &particles[idx2]);
    }
    return true;
}