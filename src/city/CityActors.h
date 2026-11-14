#pragma once

#include "../renderer/Color.h"
#include "../renderer/Renderer.h"
#include <cstddef>
#include <random>
#include <vector>

// Dynamic road and sidewalk actors: cars wander the lane graph, pedestrians
// walk their block sidewalk rings. Movement is simulation work; rendering
// happens once per frame through the shared unit primitives.
class CityActors {
public:
    // Fixed, deterministic actor set: the reference cars plus two walkers
    // per outer block ring.
    void initialize();

    void update(float deltaTime);

    void render(Renderer& renderer) const;

private:
    struct Car {
        float x;
        float z;
        int heading;       // 0 +X, 1 -X, 2 +Z, 3 -Z
        int color;
        float nextNode;    // grid coordinate of the next intersection node
        float speed;
    };

    struct Pedestrian {
        float centerX;     // absolute block ring center
        float centerZ;
        float s;           // distance travelled along the ring
        int direction;     // +1 clockwise, -1 counter-clockwise
        int color;
    };

    void wanderCar(Car& car, float step);
    void walkPedestrian(Pedestrian& ped, float step);
    void decideCarHeading(Car& car);
    void spawnPedestrians(int chunkX, int chunkZ);

    std::vector<Car> cars_;
    std::vector<Pedestrian> pedestrians_;
    std::mt19937 rng_;
};
