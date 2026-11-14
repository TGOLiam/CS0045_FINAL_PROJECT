#include "CityActors.h"

#include "../math/Matrix4.h"
#include "../renderer/PrimitiveMeshes.h"

#include <cmath>
#include <vector>

namespace {
// Road grid: centerlines at -540 .. 540 every 120 m; lane centers sit 2 m
// from the centerline (right-hand traffic, matching the reference lanes).
constexpr float halfSize = 540.0f;
constexpr float spacing = 120.0f;
constexpr float laneOffset = 2.0f;
// Heading indices: 0 +X, 1 -X, 2 +Z, 3 -Z. Yaw about +Y faces car fronts.
constexpr float headingYaw[4] = {0.0f, 180.0f, 270.0f, 90.0f};
constexpr int headingSign[4] = {1, -1, 1, -1};
constexpr bool headingAlongX[4] = {true, true, false, false};

// Sidewalk walking ring around one block; the line sits 5.5 m in from the
// road centerline, on the sidewalk slab.
constexpr float ringHalf = 54.5f;
constexpr float sideLength = 2.0f * ringHalf;
constexpr float ringPerimeter = 4.0f * sideLength;

float snapToNode(float coordinate)
{
    // Centerlines sit 60 m off the multiples of 120 (roads at -540, -420,
    // ..., 540), so snapping must preserve that offset.
    return std::round((coordinate - 60.0f) / spacing) * spacing + 60.0f;
}

float firstNodeAhead(float position, int heading)
{
    return headingSign[heading] > 0
        ? std::ceil((position - 60.0f) / spacing) * spacing + 60.0f
        : std::floor((position - 60.0f) / spacing) * spacing + 60.0f;
}

Vec3 ringPoint(float centerX, float centerZ, float s)
{
    // Walk order per quarter ring: +X along the south edge, +Z along the
    // east edge, -X along the north edge, -Z along the west edge.
    s = std::fmod(s, ringPerimeter);
    if (s < 0.0f) s += ringPerimeter;
    const int side = static_cast<int>(s / sideLength);
    const float u = s - side * sideLength;
    switch (side) {
    case 0: return {centerX - ringHalf + u, 0.0f, centerZ - ringHalf};
    case 1: return {centerX + ringHalf, 0.0f, centerZ - ringHalf + u};
    case 2: return {centerX + ringHalf - u, 0.0f, centerZ + ringHalf};
    default: return {centerX - ringHalf, 0.0f, centerZ + ringHalf - u};
    }
}

float ringYaw(float s)
{
    s = std::fmod(s, ringPerimeter);
    if (s < 0.0f) s += ringPerimeter;
    const int side = static_cast<int>(s / sideLength);
    // Face the walking direction per side: east, north, west, south.
    constexpr float sideYaw[4] = {0.0f, 270.0f, 180.0f, 90.0f};
    return sideYaw[side];
}
}

void CityActors::initialize(const ActorConfig& config)
{
    rng_.seed(20261114u);
    cars_.clear();
    pedestrians_.clear();
    carsPerChunk_ = config.carsPerChunk < 0 ? 0 : config.carsPerChunk;
    walkersPerBlock_ = config.walkersPerBlock < 0 ? 0 : config.walkersPerBlock;

    for (int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
        for (int chunkX = -1; chunkX <= 1; ++chunkX) {
            spawnCars(chunkX, chunkZ, carsPerChunk_);
            spawnPedestrians(chunkX, chunkZ);
        }
    }
}

void CityActors::spawnCars(int chunkX, int chunkZ, int count)
{
    // The first twelve match the reference street placements; any beyond
    // are dropped onto a random lane heading from the seeded state.
    struct CarStart { float x; float z; int heading; };
    const CarStart starts[] = {
        {-90, -58, 0}, {40, -62, 1}, {-150, 62, 0}, {100, 58, 1},
        {-62, -100, 2}, {-58, 100, 3}, {58, 30, 2}, {62, -30, 3},
        {-30, 62, 0}, {130, 58, 1}, {178, -100, 2}, {-178, 40, 3}
    };
    const float offsetX = chunkX * 360.0f;
    const float offsetZ = chunkZ * 360.0f;
    for (int slot = 0; slot < count; ++slot) {
        Car car{};
        car.speed = 8.0f;
        car.color = slot % 6;
        if (slot < 12) {
            car.x = offsetX + starts[slot].x;
            car.z = offsetZ + starts[slot].z;
            car.heading = starts[slot].heading;
        } else {
            std::uniform_int_distribution<int> headingPick(0, 3);
            car.heading = headingPick(rng_);
            std::uniform_int_distribution<int> linePick(0, 9);
            const float center = -halfSize + static_cast<float>(linePick(rng_)) * spacing;
            std::uniform_real_distribution<float> alongPick(-halfSize, halfSize);
            if (headingAlongX[car.heading]) {
                car.z = center + (car.heading == 0 ? laneOffset : -laneOffset);
                // Along-axis coordinate is world-space; it already covers
                // every chunk, so no chunk offset applies here.
                car.x = alongPick(rng_);
            } else {
                car.x = center - (car.heading == 2 ? laneOffset : -laneOffset);
                car.z = alongPick(rng_);
            }
        }
        car.nextNode = firstNodeAhead(
            headingAlongX[car.heading] ? car.x : car.z, car.heading);
        cars_.push_back(car);
    }
}

void CityActors::spawnPedestrians(int chunkX, int chunkZ)
{
    // walkersPerBlock walkers on the ring of every block, center block
    // included. Slots spread around the full ring; directions and colours
    // alternate so the flow never reads as a conga line.
    const float offsetX = chunkX * 360.0f;
    const float offsetZ = chunkZ * 360.0f;
    int blockIndex = 0;
    for (float blockZ : {-120.0f, 0.0f, 120.0f}) {
        for (float blockX : {-120.0f, 0.0f, 120.0f}) {
            if (walkersPerBlock_ <= 0) continue;
            for (int slot = 0; slot < walkersPerBlock_; ++slot) {
                Pedestrian ped{};
                ped.centerX = offsetX + blockX;
                ped.centerZ = offsetZ + blockZ;
                ped.s = ringPerimeter * (slot + 0.5f) / walkersPerBlock_
                      + 13.0f * static_cast<float>(blockIndex % 4);
                ped.direction = slot % 2 == 0 ? 1 : -1;
                ped.color = (blockIndex + slot) % 6;
                pedestrians_.push_back(ped);
            }
            ++blockIndex;
        }
    }
}

void CityActors::update(float deltaTime)
{
    // Fixed steps keep the wander deterministic and frame-rate independent.
    constexpr float step = 1.0f / 120.0f;
    float remaining = deltaTime;
    while (remaining >= step) {
        for (Car& car : cars_) wanderCar(car, step);
        for (Pedestrian& ped : pedestrians_) walkPedestrian(ped, step);
        remaining -= step;
    }
}

void CityActors::decideCarHeading(Car& car)
{
    // The car sits at an intersection node: nodeTravel is where it crossed
    // its own road center, nodeCross is the crossing road's centerline.
    const bool along = headingAlongX[car.heading];
    const float nodeTravel = car.nextNode;
    const float nodeCross = snapToNode(along ? car.z : car.x);
    const float nodeX = along ? nodeTravel : nodeCross;
    const float nodeZ = along ? nodeCross : nodeTravel;

    // Weighted exits: straight is five times as likely as a turn. An exit
    // is dropped when its first step leaves the city; the remaining pool is
    // never empty because at worst two inward turns survive.
    int options[4];
    int weights[4];
    int count = 0;
    for (int h = 0; h < 4; ++h) {
        const float start = headingAlongX[h] ? nodeX : nodeZ;
        const bool leavesCity = headingSign[h] > 0
            ? start >= halfSize : start <= -halfSize;
        if (leavesCity) continue;
        options[count] = h;
        weights[count] = h == car.heading ? 5 : 2;
        ++count;
    }
    int chosen = car.heading;
    if (count > 0) {
        int total = 0;
        for (int i = 0; i < count; ++i) total += weights[i];
        std::uniform_int_distribution<int> pick(0, total - 1);
        int roll = pick(rng_);
        for (int i = 0; i < count; ++i) {
            roll -= weights[i];
            if (roll < 0) {
                chosen = options[i];
                break;
            }
        }
    }

    // Snap onto the chosen lane. Lane offset direction flips for -X and -Z
    // so right-hand traffic keeps the same road half in all headings.
    const bool chosenAlongX = headingAlongX[chosen];
    const float sign = static_cast<float>(headingSign[chosen]);
    if (chosenAlongX) {
        car.x = nodeX;
        car.z = nodeZ + sign * laneOffset;
        car.nextNode = nodeX + sign * spacing;
    } else {
        car.z = nodeZ;
        car.x = nodeX - sign * laneOffset;
        car.nextNode = nodeZ + sign * spacing;
    }
    car.heading = chosen;
}

void CityActors::wanderCar(Car& car, float step)
{
    const int sign = headingSign[car.heading];
    if (headingAlongX[car.heading]) {
        car.x += sign * car.speed * step;
        if ((car.x - car.nextNode) * sign >= 0.0f) decideCarHeading(car);
    } else {
        car.z += sign * car.speed * step;
        if ((car.z - car.nextNode) * sign >= 0.0f) decideCarHeading(car);
    }
}

void CityActors::walkPedestrian(Pedestrian& ped, float step)
{
    // Bounce at ring endpoints, which reads as a casual direction change.
    ped.s += ped.direction * 1.4f * step;
    if (ped.s >= ringPerimeter) {
        ped.s = 2.0f * ringPerimeter - ped.s;
        ped.direction = -1;
    } else if (ped.s < 0.0f) {
        ped.s = -ped.s;
        ped.direction = 1;
    }
}

void CityActors::render(Renderer& renderer) const
{
    // Actors are batched into per-color dynamic VBOs: actors transform
    // primitive templates once into world-space vertex lists and every color
    // sends exactly one draw call per frame.
    const Color carColors[] = {{0.85f,0.31f,0.24f}, {0.18f,0.44f,0.71f},
                               {0.91f,0.77f,0.28f}, {0.94f,0.94f,0.94f},
                               {0.24f,0.56f,0.37f}, {0.17f,0.18f,0.21f}};
    const Color glass = {0.44f, 0.66f, 0.78f};
    const Color tires = {0.08f, 0.08f, 0.09f};
    const Color shirtColors[] = {{0.79f,0.35f,0.29f}, {0.24f,0.45f,0.62f},
                                 {0.91f,0.77f,0.33f}, {0.92f,0.92f,0.92f},
                                 {0.29f,0.55f,0.38f}, {0.25f,0.28f,0.32f}};
    const Color pants = {0.21f, 0.23f, 0.27f};
    const Color head = {0.87f, 0.68f, 0.55f};

    static const std::vector<float> cubeTemplate = PrimitiveMeshes::cube();
    static const std::vector<float> cylinderTemplate = PrimitiveMeshes::cylinder();

    // Both templates contain triangles, so geometry can share color batches.
    std::map<std::tuple<float, float, float>, std::vector<float>> batches;
    const auto emit = [&](const Matrix4& model, Color color, bool cylinder = false) {
        const std::vector<float>& source = cylinder ? cylinderTemplate : cubeTemplate;
        std::vector<float>& out = batches[{color.r, color.g, color.b}];
        const float* m = model.data();
        for (std::size_t i = 0; i < source.size(); i += 3) {
            const float x = source[i];
            const float y = source[i + 1];
            const float z = source[i + 2];
            out.push_back(m[0] * x + m[4] * y + m[8] * z + m[12]);
            out.push_back(m[1] * x + m[5] * y + m[9] * z + m[13]);
            out.push_back(m[2] * x + m[6] * y + m[10] * z + m[14]);
        }
    };

    for (const Car& car : cars_) {
        const Matrix4 carBase = Matrix4::translation(car.x, 0.05f, car.z)
                              * Matrix4::rotation(headingYaw[car.heading],
                                                  {0.0f, 1.0f, 0.0f});
        emit(carBase * Matrix4::translation(0.0f, 0.6f, 0.0f)
                    * Matrix4::scale(4.4f, 0.6f, 1.8f),
             carColors[car.color % 6]);
        emit(carBase * Matrix4::translation(-0.2f, 1.175f, 0.0f)
                    * Matrix4::scale(2.3f, 0.55f, 1.6f),
             carColors[car.color % 6]);
        for (float side : {-1.0f, 1.0f}) {
            emit(carBase * Matrix4::translation(-0.7f, 1.18f, side * 0.81f)
                        * Matrix4::scale(1.0f, 0.4f, 0.02f), glass);
        }
        for (float side : {-1.0f, 1.0f}) {
            for (float wheelX : {-1.4f, 1.4f}) {
                emit(carBase * Matrix4::translation(wheelX, 0.32f, side * 0.81f)
                            * Matrix4::scale(0.64f, 0.64f, 0.22f),
                     tires, true);
            }
        }
    }

    for (const Pedestrian& ped : pedestrians_) {
        const Vec3 position = ringPoint(ped.centerX, ped.centerZ, ped.s);
        const Matrix4 base = Matrix4::translation(position.x, 0.15f, position.z)
            * Matrix4::rotation(ringYaw(ped.s), {0.0f, 1.0f, 0.0f});
        const float sway = 0.02f * std::sin(ped.s * 0.35f);
        emit(base * Matrix4::translation(0.0f, 0.4f, 0.0f)
                    * Matrix4::scale(0.42f, 0.8f, 0.26f), pants);
        emit(base * Matrix4::translation(0.0f, 1.15f + sway, 0.0f)
                    * Matrix4::scale(0.52f, 0.7f, 0.3f),
             shirtColors[ped.color % 6]);
        emit(base * Matrix4::translation(0.0f, 1.66f + sway, 0.0f)
                    * Matrix4::scale(0.28f, 0.3f, 0.28f), head);
    }

    // Stable batch order keeps dynamic mesh ids valid across frames.
    for (const auto& entry : batches) {
        std::size_t id = 0;
        const auto found = batchMeshIds_.find(entry.first);
        if (found != batchMeshIds_.end()) {
            id = found->second;
            if (!renderer.updateDynamicMesh(id, entry.second, GL_TRIANGLES)) return;
        } else {
            if (!renderer.createDynamicMesh(entry.second, GL_TRIANGLES, id)) return;
            batchMeshIds_.emplace(entry.first, id);
        }
        renderer.drawDynamicMesh(id, Matrix4::identity(),
                                 {std::get<0>(entry.first), std::get<1>(entry.first), std::get<2>(entry.first)});
    }
}
