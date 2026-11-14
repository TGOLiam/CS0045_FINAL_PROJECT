#pragma once

#include "../renderer/Color.h"
#include "../renderer/Renderer.h"
#include <cstddef>
#include <random>
#include <vector>

// Density knobs for the dynamic city actors. Defaults keep the reference
// car placements plus a lively sidewalk population; both counts clamp to
// sensible ranges and extra cars beyond the 12 reference starts are placed
// deterministically from the actor RNG.
struct ActorConfig {
    int carsPerChunk = 28;       // ~2.3x the 12 reference starts; extras are seeded
    int walkersPerBlock = 12;    // per block sidewalk ring, all 9 blocks
};

// Dynamic road and sidewalk actors: cars wander the lane graph, pedestrians
// walk their block sidewalk rings. Movement is simulation work; rendering
// happens once per frame through the shared unit primitives.
class CityActors {
public:
    // Fixed, deterministic actor set built from the density config.
    void initialize(const ActorConfig& config = ActorConfig{});

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
    void spawnCars(int chunkX, int chunkZ, int count);
    void spawnPedestrians(int chunkX, int chunkZ);

    int carsPerChunk_ = 0;
    int walkersPerBlock_ = 0;
    std::vector<Car> cars_;
    std::vector<Pedestrian> pedestrians_;
    std::mt19937 rng_;
};
