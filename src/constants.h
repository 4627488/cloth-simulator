#ifndef CONSTANTS_H
#define CONSTANTS_H

const int WIDTH = 1920;
const int HEIGHT = 1080;
const float PARTICLE_RADIOUS = 10.0f; // Note: This seems unused in the provided main.cpp draw calls
const float GRAVITY_CONST = 10.0f; // Renamed from GRAVITY to avoid conflict
const float TIME_STEP = 0.1f;

const int DEFAULT_ROW = 60;
const int DEFAULT_COL = 60;
const float DEFAULT_REST_DISTANCE = 8.0f;

#endif // CONSTANTS_H