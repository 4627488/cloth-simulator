# Cloth Simulation

## Project Overview

This project implements a 2D cloth physics simulation system based on C++ and SFML. Users can interact with the cloth using the mouse and keyboard, enabling features such as dragging, pinning, wind, gravity adjustment, cloth reset, and more. The project adopts an object-oriented design, with a clear structure and easy extensibility.

## Main Features

- **Cloth Physics Simulation**: Realistic elasticity and flexibility effects based on particles and constraints.
- **Mouse Dragging**: Hold and drag particles on the cloth with the left mouse button for interactive pulling.
- **Pin/Unpin Particles**: Right-click particles to toggle their pinned state.
- **Wind Simulation**: Press the spacebar to toggle wind; wind strength is adjustable.
- **Gravity Adjustment**: Adjust gravity in real time using the =/- keys.
- **Cloth Reset**: Press R to reset the cloth to its initial state.
- **Cloth Tearing**: Optionally support tearing the cloth by clicking with the mouse.
- **Color Gradient**: Particles and lines display different colors based on state and force.
- **Adjustable Parameters**: Wind, gravity, and other parameters can be dynamically adjusted via keyboard.

---

## Controls

- **Left Mouse Drag**: Hold and drag particles on the cloth.
- **Right Mouse Toggle Pin**: Right-click a particle to toggle its pinned/unpinned state.
- **Spacebar**: Toggle wind.
- **[ / ] Keys**: Decrease/increase wind strength.
- **= / - Keys**: Increase/decrease gravity strength.
- **R Key**: Reset the cloth.
- **Close Window**: Click the window close button.

---

## Build & Run

### Dependencies

- C++17 or higher
- [SFML 2.5+](https://www.sfml-dev.org/) (graphics, window, and system modules required)

### Build (using CMake as example)

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Run

```bash
# Windows
.\build\bin\Debug\main.exe

# Linux/Mac
./build/bin/main
```

---

## Code Structure

- `src/main.cpp` — Main program entry
- `src/particle.h/cpp` — Particle class
- `src/constraint.h/cpp` — Constraint class
- `src/cloth.h/cpp` — Cloth class (object-oriented encapsulation)
- `src/simulation.h/cpp` — Simulation controller class (event loop, parameters, UI, etc.)
- `src/input_handler.h` — Interaction helper (e.g., mouse tearing)

---

- Fully object-oriented design, easy to extend and maintain
- Rich interactive features, supporting real-time parameter adjustment
- Detailed code comments, clear structure
- Supports large-scale cloth simulation with good performance

## Acknowledgements

- [SFML](https://www.sfml-dev.org/) graphics library
- Related open-source resources on physics simulation and computer graphics
