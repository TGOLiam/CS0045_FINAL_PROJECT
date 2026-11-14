#include "City.h"

#include "renderer/Renderer.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
#include <vector>

namespace {
constexpr float chunkSize = CITY_CHUNK_SIZE;
constexpr float cityHalfSize = 1.5f * chunkSize;
constexpr float roadSpacing = CITY_ROAD_SPACING;
constexpr float roadWidth = CITY_ROAD_WIDTH;

bool chunkHasPark(int x, int z)
{
    // Keep the starting park and the same deterministic generated variants.
    return (x == 0 && z == 0) || ((x * 17 + z * 31) % 3 + 3) % 3 == 0;
}

template <typename Callback>
void forEachTree(Callback callback)
{
    // Local positions from the reference: mirrored park trees and street trees.
    const float parkPositions[][2] = {{14,14}, {36,12}, {12,36}, {24,24},
                                      {42,24}, {24,42}, {44,44}, {10,46}};
    const float streetPositions[][2] = {{-120,-66}, {0,-66}, {120,-66}, {-120,54},
                                        {120,54}, {-66,-120}, {-66,120}, {66,-120},
                                        {66,0}, {120,66}, {120,174}, {0,174}};
    for (int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
        for (int chunkX = -1; chunkX <= 1; ++chunkX) {
            const float offsetX = chunkX * chunkSize;
            const float offsetZ = chunkZ * chunkSize;
            if (chunkHasPark(chunkX, chunkZ)) {
                for (float sx : {-1.0f, 1.0f}) {
                    for (float sz : {-1.0f, 1.0f}) {
                        for (const auto& p : parkPositions)
                            callback(offsetX + 120.0f + sx * p[0], offsetZ + 120.0f + sz * p[1]);
                    }
                }
            }
            for (const auto& p : streetPositions) callback(offsetX + p[0], offsetZ + p[1]);
        }
    }
}

enum class DecorKind { Walkway, Plaza, Garden, ServicePad, ParkingPad, Shrub, Planter, Bench, Bin };

struct DecorItem {
    DecorKind kind;
    bool lightCrown;
    float x1;
    float z1;
    float x2;
    float z2;
};

struct DecorBlock {
    char key;
    std::size_t count;
    const DecorItem* items;
};

// Cars and pedestrians are dynamic actors at runtime (src/city/CityActors);
// they are no longer baked into the static city asset.

// Block-detail placements from the design reference (metres, chunk-local).
// Flat items (walkway, plaza, garden, pads) store corner extents [x1,z1,x2,z2];
// props (shrub, planter, bench, bin) store a center point and lightCrown only
// matters for shrubs. Park chunks drop the J set together with buildings I1/I2.
constexpr DecorItem decorBlockA[] = {
    {DecorKind::Plaza, false, -148.0f, -123.0f, -136.0f, -109.0f},
    {DecorKind::Garden, false, -153.0f, -76.5f, -126.0f, -70.5f},
    {DecorKind::Garden, false, -113.0f, -76.5f, -87.0f, -70.5f},
    {DecorKind::Walkway, false, -83.0f, -93.2f, -67.0f, -90.8f},
    {DecorKind::Walkway, false, -121.1f, -77.0f, -118.9f, -67.0f},
    {DecorKind::Plaza, false, -97.0f, -131.0f, -86.0f, -118.0f},
    {DecorKind::Bench, false, -95.5f, -125.0f, -92.5f, -124.4f},
    {DecorKind::Walkway, false, -73.0f, -147.2f, -67.0f, -144.8f},
    {DecorKind::Shrub, false, -148.0f, -74.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, -136.0f, -74.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, -124.0f, -74.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, false, -108.0f, -74.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, -95.0f, -74.0f, 0.0f, 0.0f},
    {DecorKind::Bin, false, -90.0f, -70.5f, 0.0f, 0.0f},
    {DecorKind::ServicePad, false, -103.0f, -172.5f, -81.0f, -160.5f},
    {DecorKind::Planter, false, -101.0f, -168.0f, 0.0f, 0.0f},
    {DecorKind::Bin, false, -96.0f, -166.0f, 0.0f, 0.0f},
    {DecorKind::ParkingPad, false, -123.0f, -158.0f, -111.0f, -124.0f},
};
constexpr DecorItem decorBlockB[] = {
    {DecorKind::Plaza, false, -33.0f, -125.0f, -17.0f, -117.0f},
    {DecorKind::Walkway, false, -45.2f, -125.0f, -42.8f, -67.0f},
    {DecorKind::Shrub, false, -15.0f, -120.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, -15.0f, -110.0f, 0.0f, 0.0f},
    {DecorKind::Planter, false, -35.0f, -120.0f, 0.0f, 0.0f},
    {DecorKind::Planter, false, -35.0f, -110.0f, 0.0f, 0.0f},
    {DecorKind::Garden, false, 2.0f, -126.0f, 42.0f, -116.0f},
    {DecorKind::Shrub, false, 10.0f, -121.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, false, 22.0f, -121.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, 34.0f, -118.0f, 0.0f, 0.0f},
    {DecorKind::Walkway, false, 42.9f, -126.0f, 45.1f, -67.0f},
    {DecorKind::Walkway, false, -1.5f, -78.0f, 1.5f, -67.0f},
    {DecorKind::Bin, false, -36.0f, -71.0f, 0.0f, 0.0f},
    {DecorKind::Planter, false, 36.0f, -71.0f, 0.0f, 0.0f},
    {DecorKind::ParkingPad, false, -8.0f, -159.0f, 2.0f, -126.0f},
};
constexpr DecorItem decorBlockC[] = {
    {DecorKind::Walkway, false, 70.0f, -141.2f, 72.5f, -67.0f},
    {DecorKind::Plaza, false, 80.0f, -118.0f, 100.0f, -106.0f},
    {DecorKind::Plaza, false, 135.0f, -129.0f, 153.0f, -115.0f},
    {DecorKind::Walkway, false, 160.0f, -145.2f, 162.5f, -67.0f},
    {DecorKind::Planter, false, 165.0f, -122.0f, 0.0f, 0.0f},
    {DecorKind::Planter, false, 165.0f, -136.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, false, 80.0f, -104.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, 100.0f, -104.0f, 0.0f, 0.0f},
    {DecorKind::Walkway, false, 118.7f, -77.0f, 121.3f, -67.0f},
    {DecorKind::Shrub, false, 108.0f, -72.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, 132.0f, -72.0f, 0.0f, 0.0f},
    {DecorKind::Bin, false, 126.0f, -70.5f, 0.0f, 0.0f},
    {DecorKind::ServicePad, false, 115.0f, -172.5f, 131.5f, -160.5f},
    {DecorKind::Planter, false, 118.0f, -168.0f, 0.0f, 0.0f},
    {DecorKind::Bin, false, 125.0f, -166.0f, 0.0f, 0.0f},
};
constexpr DecorItem decorBlockD[] = {
    {DecorKind::Walkway, false, -123.0f, -1.2f, -67.0f, 1.2f},
    {DecorKind::ParkingPad, false, -123.0f, -28.0f, -115.0f, 28.0f},
    {DecorKind::Bin, false, -119.0f, -31.0f, 0.0f, 0.0f},
    {DecorKind::Plaza, false, -104.0f, -3.0f, -88.0f, 5.0f},
    {DecorKind::Walkway, false, -76.2f, -39.0f, -73.8f, 53.0f},
    {DecorKind::Bin, false, -72.0f, 50.0f, 0.0f, 0.0f},
    {DecorKind::Walkway, false, -97.1f, 42.0f, -94.9f, 53.0f},
    {DecorKind::Shrub, false, -102.0f, 48.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, -90.0f, 48.0f, 0.0f, 0.0f},
};
constexpr DecorItem decorBlockE[] = {
    {DecorKind::Plaza, false, -13.0f, 15.0f, 13.0f, 31.0f},
    {DecorKind::Walkway, false, -2.0f, 31.0f, 2.0f, 53.0f},
    {DecorKind::Walkway, false, -35.1f, -25.0f, -32.9f, 53.0f},
    {DecorKind::Garden, false, -46.0f, -26.0f, -22.5f, -17.0f},
    {DecorKind::Shrub, false, -42.0f, -21.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, -27.0f, -18.0f, 0.0f, 0.0f},
    {DecorKind::Walkway, false, 32.9f, 47.0f, 35.1f, 53.0f},
    {DecorKind::Shrub, false, 28.0f, 50.0f, 0.0f, 0.0f},
    {DecorKind::Bin, false, 41.9f, 50.0f, 0.0f, 0.0f},
    {DecorKind::Bench, false, -16.0f, 24.0f, -15.4f, 28.0f},
    {DecorKind::Bench, false, 15.4f, 24.0f, 16.0f, 28.0f},
    {DecorKind::Shrub, false, -9.0f, 38.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, -9.0f, 46.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, false, 9.0f, 38.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, 9.0f, 46.0f, 0.0f, 0.0f},
    {DecorKind::Planter, false, -8.0f, 51.0f, 0.0f, 0.0f},
    {DecorKind::Planter, false, 8.0f, 51.0f, 0.0f, 0.0f},
    {DecorKind::Bin, false, 4.6f, 49.4f, 0.0f, 0.0f},
};
constexpr DecorItem decorBlockF[] = {
    {DecorKind::Walkway, false, 92.8f, -2.0f, 95.2f, 53.0f},
    {DecorKind::Garden, false, 78.0f, 2.0f, 110.0f, 10.0f},
    {DecorKind::Shrub, false, 84.0f, 6.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, 96.0f, 6.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, false, 104.0f, 6.0f, 0.0f, 0.0f},
    {DecorKind::Walkway, false, 143.8f, -6.0f, 146.2f, 53.0f},
    {DecorKind::Garden, false, 122.0f, -52.0f, 166.0f, -44.0f},
    {DecorKind::Shrub, false, 128.0f, -48.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, 160.0f, -48.0f, 0.0f, 0.0f},
    {DecorKind::Plaza, false, 105.0f, 45.0f, 135.0f, 53.0f},
    {DecorKind::Bench, false, 107.6f, 49.0f, 108.2f, 52.0f},
    {DecorKind::Bench, false, 131.4f, 49.0f, 132.0f, 52.0f},
    {DecorKind::Planter, false, 102.0f, 48.0f, 0.0f, 0.0f},
    {DecorKind::Planter, false, 138.0f, 48.0f, 0.0f, 0.0f},
    {DecorKind::Bin, false, 119.0f, 51.0f, 0.0f, 0.0f},
};
constexpr DecorItem decorBlockG[] = {
    {DecorKind::Plaza, false, -152.0f, 114.0f, -136.0f, 126.0f},
    {DecorKind::Shrub, false, -150.0f, 116.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, -138.0f, 116.0f, 0.0f, 0.0f},
    {DecorKind::Walkway, false, -173.0f, 96.8f, -161.0f, 99.2f},
    {DecorKind::Walkway, false, -166.2f, 99.0f, -163.8f, 67.0f},
    {DecorKind::Bin, false, -70.0f, 88.0f, 0.0f, 0.0f},
    {DecorKind::Walkway, false, -75.0f, 92.8f, -67.0f, 95.2f},
    {DecorKind::Garden, false, -112.0f, 110.0f, -76.0f, 118.0f},
    {DecorKind::Shrub, true, -106.0f, 114.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, -82.0f, 114.0f, 0.0f, 0.0f},
    {DecorKind::Garden, false, -152.0f, 165.0f, -88.0f, 172.0f},
    {DecorKind::Walkway, false, -121.2f, 164.0f, -118.8f, 173.0f},
    {DecorKind::Shrub, true, -146.0f, 168.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, false, -94.0f, 168.0f, 0.0f, 0.0f},
};
constexpr DecorItem decorBlockH[] = {
    {DecorKind::Walkway, false, -27.2f, 79.0f, -24.8f, 67.0f},
    {DecorKind::Bin, false, -44.0f, 72.0f, 0.0f, 0.0f},
    {DecorKind::Garden, false, 6.0f, 116.0f, 42.0f, 130.0f},
    {DecorKind::Shrub, false, 12.0f, 123.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, 24.0f, 123.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, false, 36.0f, 123.0f, 0.0f, 0.0f},
    {DecorKind::Walkway, false, 44.0f, 94.8f, 53.0f, 97.2f},
    {DecorKind::Shrub, false, 49.0f, 92.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, 49.0f, 100.0f, 0.0f, 0.0f},
    {DecorKind::Garden, false, -14.0f, 164.0f, 14.0f, 172.0f},
    {DecorKind::Walkway, false, -1.2f, 164.0f, 1.2f, 173.0f},
    {DecorKind::Bench, false, 6.0f, 169.9f, 8.0f, 170.4f},
    {DecorKind::Planter, false, -9.0f, 170.0f, 0.0f, 0.0f},
};
constexpr DecorItem decorBlockJ[] = {
    {DecorKind::Walkway, false, 94.8f, 121.0f, 97.2f, 173.0f},
    {DecorKind::Bench, false, 95.2f, 145.0f, 97.2f, 146.0f},
    {DecorKind::Shrub, false, 88.0f, 133.0f, 0.0f, 0.0f},
    {DecorKind::Shrub, true, 104.0f, 133.0f, 0.0f, 0.0f},
    {DecorKind::Walkway, false, 142.9f, 156.0f, 145.1f, 173.0f},
    {DecorKind::Bin, false, 150.0f, 166.0f, 0.0f, 0.0f},
};

constexpr DecorBlock decorBlocks[] = {
    {'A', std::size(decorBlockA), decorBlockA},
    {'B', std::size(decorBlockB), decorBlockB},
    {'C', std::size(decorBlockC), decorBlockC},
    {'D', std::size(decorBlockD), decorBlockD},
    {'E', std::size(decorBlockE), decorBlockE},
    {'F', std::size(decorBlockF), decorBlockF},
    {'G', std::size(decorBlockG), decorBlockG},
    {'H', std::size(decorBlockH), decorBlockH},
    {'J', std::size(decorBlockJ), decorBlockJ},
};

template <typename Callback>
void forEachDecor(Callback callback)
{
    // The J block replaces the park's southeast lot, so it only exists where
    // the generator placed I1/I2; every other block set is fixed.
    for (const DecorBlock& block : decorBlocks) {
        for (int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
            for (int chunkX = -1; chunkX <= 1; ++chunkX) {
                if (block.key == 'J' && chunkHasPark(chunkX, chunkZ)) continue;
                const float offsetX = chunkX * chunkSize;
                const float offsetZ = chunkZ * chunkSize;
                for (const DecorItem* it = block.items; it != block.items + block.count; ++it)
                    callback(*it, offsetX, offsetZ);
            }
        }
    }
}

enum class BuildingStyle { Tower, Residential, Commercial };

struct Building {
    float x;
    float z;
    float width;
    float depth;
    float height;
    Color color;
    BuildingStyle style;
};

constexpr Color towerBlue = {93.0f/255.0f, 122.0f/255.0f, 153.0f/255.0f};
constexpr Color towerGreen = {106.0f/255.0f, 143.0f/255.0f, 143.0f/255.0f};
constexpr Color towerPurple = {125.0f/255.0f, 122.0f/255.0f, 160.0f/255.0f};
constexpr Color residentialClay = {201.0f/255.0f, 142.0f/255.0f, 107.0f/255.0f};
constexpr Color residentialSand = {212.0f/255.0f, 163.0f/255.0f, 115.0f/255.0f};
constexpr Color residentialRose = {181.0f/255.0f, 119.0f/255.0f, 110.0f/255.0f};
constexpr Color commercialYellow = {224.0f/255.0f, 201.0f/255.0f, 122.0f/255.0f};
constexpr Color commercialGreen = {199.0f/255.0f, 211.0f/255.0f, 160.0f/255.0f};
constexpr Color commercialPeach = {230.0f/255.0f, 160.0f/255.0f, 140.0f/255.0f};

// Starting chunk footprints from the design reference. The center tower is
// drawn separately, and the southeast block is reserved for the park.
#include "city_layout_generated.h"

void drawSurface(Renderer& renderer, float x, float z, float width, float depth,
                 float height, Color color)
{
    const Matrix4 model = Matrix4::translation(x, height, z)
                        * Matrix4::rotation(-90.0f, {1.0f, 0.0f, 0.0f})
                        * Matrix4::scale(width, depth, 1.0f);
    renderer.draw(MeshId::Quad, model, color);
}

void drawRoadLine(Renderer& renderer, float x, float z, float length,
                  bool alongZ, Color color)
{
    const Matrix4 model = Matrix4::translation(x, 0.08f, z)
                        * Matrix4::rotation(alongZ ? 90.0f : 0.0f, {0.0f, 1.0f, 0.0f})
                        * Matrix4::scale(length, 1.0f, 1.0f);
    renderer.draw(MeshId::Line, model, color);
}

void drawBox(Renderer& renderer, float x, float y, float z,
             float width, float height, float depth, Color color)
{
    const Matrix4 model = Matrix4::translation(x, y, z)
                        * Matrix4::scale(width, height, depth);
    renderer.draw(MeshId::Cube, model, color);
}

struct FacadeDetail {
    Matrix4 model;
    Color color;
};

std::vector<FacadeDetail> makeFacadeDetails(const std::vector<Building>& buildings)
{
    std::vector<FacadeDetail> details;
    for (const Building& b : buildings) {
        const bool tower = b.style == BuildingStyle::Tower;
        const bool residential = b.style == BuildingStyle::Residential;
        const float groundHeight = residential ? 4.2f : 4.5f;
        const float floorHeight = tower ? 3.5f : (residential ? 3.2f : 3.4f);
        const float windowWidth = tower ? 2.0f : (residential ? 1.4f : 1.6f);
        const float windowHeight = tower ? 2.2f : (residential ? 1.6f : 1.5f);
        const float sill = tower ? 0.6f : 0.9f;
        const Color window = tower ? Color{0.66f, 0.86f, 0.94f}
                                   : Color{0.53f, 0.75f, 0.86f};
        const Color trim = tower ? Color{0.18f, 0.25f, 0.31f}
                                : (residential ? Color{0.54f, 0.42f, 0.33f}
                                               : Color{0.43f, 0.42f, 0.39f});
        for (int face = 0; face < 4; ++face) {
            const bool xFace = face >= 2;
            const float sign = (face % 2 == 0) ? 1.0f : -1.0f;
            const float span = xFace ? b.depth : b.width;
            const float pitch = tower ? 3.0f : (residential ? (xFace ? 6.0f : 4.0f) : 3.2f);
            const int bays = static_cast<int>(std::floor((span - 2.4f) / pitch));
            const int floors = static_cast<int>(std::round((b.height - groundHeight) / floorHeight));
            // The shared quad is XY-facing +Z; rotate it for the X walls.
            const Matrix4 wall = Matrix4::translation(
                b.x + (xFace ? sign * (b.width / 2.0f + 0.02f) : 0.0f),
                0.0f, b.z + (xFace ? 0.0f : sign * (b.depth / 2.0f + 0.02f)))
                * Matrix4::rotation(xFace ? 90.0f : 0.0f, {0.0f, 1.0f, 0.0f});
            auto add = [&](float u, float y, float width, float height, Color color) {
                details.push_back({wall * Matrix4::translation(u, y, 0.0f)
                                       * Matrix4::scale(width, height, 1.0f), color});
            };
            for (int floor = 0; floor < floors; ++floor) {
                const float y = groundHeight + floor * floorHeight + sill + windowHeight / 2.0f;
                if (y + windowHeight / 2.0f > b.height) continue;
                for (int bay = 0; bay < bays; ++bay) {
                    const float u = (bay - (bays - 1) / 2.0f) * pitch;
                    // Residential service side has a central stairwell strip.
                    if (residential && face == 3 && std::abs(u) < 1.2f) continue;
                    add(u, y, windowWidth, windowHeight, window);
                    if (residential && face == 0)
                        add(u, groundHeight + floor * floorHeight + 0.675f,
                            windowWidth + 0.8f, 0.25f, trim);
                }
            }
            const Color door = {0.14f, 0.15f, 0.18f};
            if (tower) {
                const float doorWidth = face == 0 ? 3.6f : (face == 1 ? 1.8f : (face == 2 ? 1.6f : 1.4f));
                add(0.0f, face == 0 ? 1.5f : 1.2f, doorWidth, face == 0 ? 3.0f : 2.4f, door);
                if (face == 0) add(0.0f, 3.45f, 5.6f, 0.3f, trim);
                if (face == 1) add(span * 0.2f, 1.4f, 3.0f, 1.2f, trim);
                if (face == 3) {
                    for (int louver = 0; louver < 3; ++louver)
                        add(0.0f, 0.95f + louver * 0.6f, span - 10.0f, 0.2f, trim);
                }
            } else if (residential) {
                const float doorWidth = face == 0 ? 2.4f : 1.2f;
                add(face == 3 ? span * 0.3f : 0.0f, 1.2f, doorWidth, 2.4f, door);
                if (face == 3) add(0.0f, (groundHeight + b.height) / 2.0f,
                                   1.0f, b.height - groundHeight, trim);
                if (face == 1) add(span * 0.2f, 1.5f, 1.6f, 0.6f, trim);
            } else {
                if (face == 0) {
                    add(-span / 2.0f + 1.9f, 1.3f, 1.8f, 2.6f, door);
                    // Shop glazing starts after the entrance and repeats in bays.
                    for (float u = -span / 2.0f + 4.8f; u + 1.5f < span / 2.0f - 1.0f; u += 3.2f)
                        add(u, 1.7f, 3.0f, 2.6f, window);
                    add(0.0f, 3.25f, span - 1.0f, 0.5f, {0.78f, 0.32f, 0.24f});
                    add(0.0f, 4.05f, span - 1.0f, 0.7f, trim);
                } else if (face == 1) {
                    add(-span * 0.2f, 1.2f, 1.6f, 2.4f, door);
                    add(span * 0.1f, 1.4f, 2.6f, 2.8f, door);
                } else if (face == 2) {
                    add(0.0f, 1.1f, 1.2f, 2.2f, door);
                }
            }
        }
    }
    return details;
}

std::vector<Building> makeBuildings()
{
    std::vector<Building> buildings(std::begin(startingBuildings), std::end(startingBuildings));
    const Color towerColors[] = {towerBlue, towerGreen, towerPurple};
    const Color residentialColors[] = {residentialClay, residentialSand, residentialRose};
    const Color commercialColors[] = {commercialYellow, commercialGreen, commercialPeach};

    for (int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
        for (int chunkX = -1; chunkX <= 1; ++chunkX) {
            if (chunkX == 0 && chunkZ == 0) continue;
            // Match the reference's deterministic chunk variants, including
            // its normalized modulo for negative coordinates.
            const int variant = ((chunkX * 17 + chunkZ * 31) % 3 + 3) % 3;
            const bool hasPark = chunkHasPark(chunkX, chunkZ);
            const float offsetX = chunkX * chunkSize;
            const float offsetZ = chunkZ * chunkSize;
            auto append = [&](Building b, int index) {
                const float ground = b.style == BuildingStyle::Residential ? 4.2f : 4.5f;
                const float floor = b.style == BuildingStyle::Tower ? 3.5f :
                                    (b.style == BuildingStyle::Residential ? 3.2f : 3.4f);
                const float desired = std::min(45.0f, b.height * (0.55f + variant * 0.1f));
                b.height = ground + std::max(1.0f, std::round((desired - ground) / floor)) * floor;
                const int colorIndex = (index + variant) % 3;
                b.color = b.style == BuildingStyle::Tower ? towerColors[colorIndex] :
                          (b.style == BuildingStyle::Residential ? residentialColors[colorIndex] : commercialColors[colorIndex]);
                b.x += offsetX;
                b.z += offsetZ;
                buildings.push_back(b);
            };
            append({0, 0, 30, 30, 25.0f, towerBlue, BuildingStyle::Tower}, 12);
            int index = 0;
            for (const Building& b : startingBuildings) {
                const int paletteIndex = index < 12 ? index : index + 1;
                append(b, paletteIndex);
                ++index;
            }
            if (!hasPark) {
                Building residential = {96, 100, 36, 40, 23.4f, residentialColors[variant], BuildingStyle::Residential};
                Building commercial = {144, 140, 32, 30, 11.3f, commercialColors[variant], BuildingStyle::Commercial};
                residential.x += offsetX;
                residential.z += offsetZ;
                commercial.x += offsetX;
                commercial.z += offsetZ;
                buildings.push_back(residential);
                buildings.push_back(commercial);
            }
        }
    }
    return buildings;
}

const std::vector<Building>& cityBuildings()
{
    static const std::vector<Building> buildings = makeBuildings();
    return buildings;
}

std::vector<Aabb> makeCollisionBounds()
{
    std::vector<Aabb> boxes;
    auto box = [&](float x, float y, float z, float w, float h, float d) {
        boxes.push_back({{x-w/2, y-h/2, z-d/2}, {x+w/2, y+h/2, z+d/2}});
    };
    for (const Building& b : cityBuildings()) {
        box(b.x, b.height/2, b.z, b.width, b.height, b.depth);
        for (float side : {-1.0f, 1.0f}) {
            box(b.x, b.height+0.4f, b.z+side*(b.depth/2-0.25f), b.width, 0.8f, 0.5f);
            box(b.x+side*(b.width/2-0.25f), b.height+0.4f, b.z, 0.5f, 0.8f, b.depth-1.0f);
        }
        if (b.style == BuildingStyle::Tower) {
            box(b.x-b.width/2+4, b.height+1.25f, b.z-b.depth/2+3.5f, 6, 2.5f, 4);
            box(b.x+b.width/2-2.5f, b.height+1.5f, b.z+b.depth/2-2.5f, 2.4f, 3, 2.4f);
        } else if (b.style == BuildingStyle::Residential) {
            box(b.x, b.height+1.2f, b.z-b.depth/4, 3, 2.4f, 3);
            for (float side : {-1.0f, 1.0f})
                box(b.x+side*b.width/4, b.height+1.2f, b.z+b.depth/4, 1, 2.4f, 1);
        } else {
            const int count = std::max(1, static_cast<int>(b.width/12));
            for (int i=0; i<count; ++i)
                box(b.x+(i-(count-1)/2.0f)*6, b.height+0.6f, b.z+b.depth/4, 2, 1.2f, 1.5f);
        }
    }
    // Central tower, parapets, canopy and rooftop equipment; include the
    // helipad markings' height in the roof collision surface.
    const float h = CITY_TOWER_HEIGHT;
    box(0, (h+0.09f)/2, 0, 30, h+0.09f, 30);
    for (float side : {-1.0f, 1.0f}) {
        box(0,h+0.4f,side*14.75f,30,0.8f,0.5f);
        box(side*14.75f,h+0.4f,0,0.5f,0.8f,29);
    }
    box(0,3.45f,15.6f,5.6f,0.3f,1.2f);
    box(-11,h+1.25f,-11.5f,6,2.5f,4);
    box(12.5f,h+1.5f,12.5f,2.4f,3,2.4f);
    // Trees use simple trunk collision; foliage remains visual geometry.
    forEachTree([&](float x, float z) { box(x, 1.0f, z, 0.7f, 2.0f, 0.7f); });
    // Street furniture gets small AABBs; shrubs stay visual so they remain
    // passable foliage and pads are flat ground shading.
    forEachDecor([&](const DecorItem& item, float ox, float oz) {
        if (item.kind == DecorKind::Planter)
            box(ox + item.x1, 0.45f, oz + item.z1, 2.0f, 0.95f, 1.2f);
        else if (item.kind == DecorKind::Bin)
            box(ox + item.x1, 0.5f, oz + item.z1, 0.75f, 1.0f, 0.75f);
        else if (item.kind == DecorKind::Bench)
            box(ox + (item.x1 + item.x2) / 2.0f, 0.275f,
                oz + (item.z1 + item.z2) / 2.0f,
                std::abs(item.x2 - item.x1), 0.55f, std::abs(item.z2 - item.z1));
    });
    return boxes;
}

void drawBuildingRoof(Renderer& renderer, const Building& b)
{
    const bool tower = b.style == BuildingStyle::Tower;
    const bool residential = b.style == BuildingStyle::Residential;
    const Color trim = tower ? Color{0.18f, 0.25f, 0.31f}
                            : (residential ? Color{0.54f, 0.42f, 0.33f}
                                           : Color{0.43f, 0.42f, 0.39f});
    // Four low parapets outline the roof without covering its surface.
    for (float side : {-1.0f, 1.0f}) {
        drawBox(renderer, b.x, b.height + 0.4f, b.z + side * (b.depth / 2.0f - 0.25f),
                b.width, 0.8f, 0.5f, trim);
        drawBox(renderer, b.x + side * (b.width / 2.0f - 0.25f), b.height + 0.4f, b.z,
                0.5f, 0.8f, b.depth - 1.0f, trim);
    }
    if (tower) {
        drawBox(renderer, b.x - b.width / 2.0f + 4.0f, b.height + 1.25f,
                b.z - b.depth / 2.0f + 3.5f, 6.0f, 2.5f, 4.0f, {0.54f, 0.58f, 0.63f});
        drawBox(renderer, b.x + b.width / 2.0f - 2.5f, b.height + 1.5f,
                b.z + b.depth / 2.0f - 2.5f, 2.4f, 3.0f, 2.4f, trim);
    } else if (residential) {
        drawBox(renderer, b.x, b.height + 1.2f, b.z - b.depth / 4.0f,
                3.0f, 2.4f, 3.0f, trim);
        for (float side : {-1.0f, 1.0f})
            drawBox(renderer, b.x + side * b.width / 4.0f, b.height + 1.2f,
                    b.z + b.depth / 4.0f, 1.0f, 2.4f, 1.0f, trim);
    } else {
        const int units = std::max(1, static_cast<int>(b.width / 12.0f));
        for (int i = 0; i < units; ++i)
            drawBox(renderer, b.x + (i - (units - 1) / 2.0f) * 6.0f,
                    b.height + 0.6f, b.z + b.depth / 4.0f, 2.0f, 1.2f, 1.5f, trim);
    }
}

void drawDiscAt(Renderer& renderer, float x, float y, float z, float diameter, Color color)
{
    const Matrix4 model = Matrix4::translation(x, y, z)
                        * Matrix4::rotation(-90.0f, {1.0f, 0.0f, 0.0f})
                        * Matrix4::scale(diameter, diameter, 1.0f);
    renderer.draw(MeshId::Disc, model, color);
}

void drawDisc(Renderer& renderer, float y, float diameter, Color color)
{
    drawDiscAt(renderer, 0.0f, y, 0.0f, diameter, color);
}
}

std::vector<Aabb> City::collisionBounds() const
{
    return makeCollisionBounds();
}

bool City::initialize(Renderer& renderer)
{
    struct BatchData {
        Color color;
        std::vector<float> vertices;
    };
    // Group by chunk and solid color so the static facade quads can share draws.
    using BatchKey = std::tuple<int, int, int, int, int>;
    std::map<BatchKey, BatchData> groups;
    const auto details = makeFacadeDetails(cityBuildings());
    constexpr float quad[] = {
        -0.5f,-0.5f, 0.5f,-0.5f, 0.5f,0.5f,
        -0.5f,-0.5f, 0.5f,0.5f, -0.5f,0.5f
    };
    for (const FacadeDetail& detail : details) {
        const float* m = detail.model.data();
        const int chunkX = static_cast<int>(std::floor((m[12] + chunkSize / 2.0f) / chunkSize));
        const int chunkZ = static_cast<int>(std::floor((m[14] + chunkSize / 2.0f) / chunkSize));
        const BatchKey key = {chunkX, chunkZ,
            static_cast<int>(std::round(detail.color.r * 255.0f)),
            static_cast<int>(std::round(detail.color.g * 255.0f)),
            static_cast<int>(std::round(detail.color.b * 255.0f))};
        BatchData& batch = groups[key];
        batch.color = detail.color;
        for (int i = 0; i < 12; i += 2) {
            const float x = quad[i];
            const float y = quad[i + 1];
            batch.vertices.push_back(m[0] * x + m[4] * y + m[12]);
            batch.vertices.push_back(m[1] * x + m[5] * y + m[13]);
            batch.vertices.push_back(m[2] * x + m[6] * y + m[14]);
        }
    }
    facadeBatches_.clear();
    for (const auto& entry : groups) {
        std::size_t id = 0;
        if (!renderer.uploadStaticMesh(entry.second.vertices, GL_TRIANGLES, id)) return false;
        facadeBatches_.push_back({id, entry.second.color});
    }
    return true;
}

void City::render(Renderer& renderer) const
{
    const Color ground = {0.85f, 0.83f, 0.77f};
    const Color asphalt = {0.27f, 0.29f, 0.32f};

    // Nine fixed chunks centered on the starting chunk; each shares its edges.
    for (int z = -1; z <= 1; ++z) {
        for (int x = -1; x <= 1; ++x) {
            drawSurface(renderer, x * chunkSize, z * chunkSize,
                        chunkSize, chunkSize, 0.0f, ground);
        }
    }

    // Park block from the reference: lawn, cross paths, a ring path, plaza,
    // and a pond. Small height offsets prevent coplanar depth flicker.
    constexpr float pi = 3.14159265358979323846f;
    const Color lawn = {127.0f/255.0f, 174.0f/255.0f, 107.0f/255.0f};
    const Color path = {234.0f/255.0f, 223.0f/255.0f, 191.0f/255.0f};
    const Color water = {127.0f/255.0f, 183.0f/255.0f, 217.0f/255.0f};
    for (int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
        for (int chunkX = -1; chunkX <= 1; ++chunkX) {
            if (!chunkHasPark(chunkX, chunkZ)) continue;
            const float x = chunkX * chunkSize + 120.0f;
            const float z = chunkZ * chunkSize + 120.0f;
            drawSurface(renderer, x, z, 106.0f, 106.0f, 0.01f, lawn);
            drawSurface(renderer, x, z, 4.0f, 106.0f, 0.02f, path);
            drawSurface(renderer, x, z, 106.0f, 4.0f, 0.02f, path);
            // Twenty-four short quads approximate the circular 3-unit path.
            constexpr int segments = 24;
            constexpr float radius = 28.0f;
            const float segmentLength = 2.0f * radius * std::tan(pi / segments);
            for (int i = 0; i < segments; ++i) {
                const float angle = 2.0f * pi * i / segments;
                const Matrix4 ringSegment =
                    Matrix4::translation(x + radius * std::cos(angle), 0.03f,
                                         z + radius * std::sin(angle))
                    * Matrix4::rotation(-angle * 180.0f / pi, {0.0f, 1.0f, 0.0f})
                    * Matrix4::rotation(-90.0f, {1.0f, 0.0f, 0.0f})
                    * Matrix4::scale(3.0f, segmentLength, 1.0f);
                renderer.draw(MeshId::Quad, ringSegment, path);
            }
            drawDiscAt(renderer, x, 0.04f, z, 20.0f, path);
            drawDiscAt(renderer, x, 0.05f, z, 10.0f, water);
        }
    }

    // Trees reuse a cube trunk and the eight-sided cone mesh. The exporter
    // batches their solid-colored triangles into the static city asset.
    forEachTree([&](float x, float z) {
        drawBox(renderer, x, 1.0f, z, 0.7f, 2.0f, 0.7f, {0.42f, 0.29f, 0.18f});
        const Matrix4 lower = Matrix4::translation(x, 1.6f, z)
                            * Matrix4::scale(3.6f, 2.6f, 3.6f);
        const Matrix4 upper = Matrix4::translation(x, 3.4f, z)
                            * Matrix4::scale(2.4f, 2.2f, 2.4f);
        renderer.draw(MeshId::Cone, lower, {0.25f, 0.49f, 0.29f});
        renderer.draw(MeshId::Cone, upper, {0.31f, 0.60f, 0.35f});
    });

    // Draw the connected grid over the whole area. Boundary roads are half
    // width; the matching half would belong to the next chunk outside this view.
    constexpr int roadCount = 10;
    for (int i = 0; i < roadCount; ++i) {
        const float center = -cityHalfSize + i * roadSpacing;
        const bool edge = i == 0 || i == roadCount - 1;
        const float width = edge ? roadWidth / 2.0f : roadWidth;
        const float clippedCenter = center + (i == 0 ? roadWidth / 4.0f :
                                             i == roadCount - 1 ? -roadWidth / 4.0f : 0.0f);
        // North-south strips own the intersections.
        drawSurface(renderer, clippedCenter, 0.0f, width, 2.0f * cityHalfSize, 0.05f, asphalt);

        // East-west strips stop at the north-south strips to avoid overlap.
        for (int j = 0; j < roadCount - 1; ++j) {
            const float start = -cityHalfSize + j * roadSpacing + roadWidth / 2.0f;
            const float end = -cityHalfSize + (j + 1) * roadSpacing - roadWidth / 2.0f;
            drawSurface(renderer, (start + end) / 2.0f, clippedCenter,
                        end - start, width, 0.05f, asphalt);
        }
    }

    // Sidewalks outline each block and meet at their edges without overlap.
    const Color sidewalk = {0.72f, 0.70f, 0.64f};
    constexpr float sidewalkWidth = 3.0f;
    constexpr float blockWidth = roadSpacing - roadWidth;
    for (int z = 0; z < roadCount - 1; ++z) {
        for (int x = 0; x < roadCount - 1; ++x) {
            const float blockX = -cityHalfSize + (x + 0.5f) * roadSpacing;
            const float blockZ = -cityHalfSize + (z + 0.5f) * roadSpacing;
            const float offset = (blockWidth - sidewalkWidth) / 2.0f;
            drawSurface(renderer, blockX, blockZ - offset, blockWidth, sidewalkWidth, 0.15f, sidewalk);
            drawSurface(renderer, blockX, blockZ + offset, blockWidth, sidewalkWidth, 0.15f, sidewalk);
            drawSurface(renderer, blockX - offset, blockZ, sidewalkWidth, blockWidth - 2.0f * sidewalkWidth, 0.15f, sidewalk);
            drawSurface(renderer, blockX + offset, blockZ, sidewalkWidth, blockWidth - 2.0f * sidewalkWidth, 0.15f, sidewalk);
        }
    }

    // Block details from the design reference: warm paving, grass strips and
    // pads fill the gaps between buildings. Heights keep the reference layer
    // stack (ground 0, asphalt 0.05, sidewalk 0.15) clear of z-fighting:
    // pads 0.065, bay lines 0.075, walkways/plazas 0.16, gardens 0.18.
    const Color walkway = {213.0f/255.0f, 203.0f/255.0f, 186.0f/255.0f};
    const Color plaza = {203.0f/255.0f, 194.0f/255.0f, 180.0f/255.0f};
    const Color grassPatch = {126.0f/255.0f, 161.0f/255.0f, 89.0f/255.0f};
    const Color servicePad = {179.0f/255.0f, 172.0f/255.0f, 157.0f/255.0f};
    const Color parkingPad = {60.0f/255.0f, 66.0f/255.0f, 75.0f/255.0f};
    const Color bayLine = {242.0f/255.0f, 242.0f/255.0f, 242.0f/255.0f};
    constexpr float baySpacing = 2.6f;
    constexpr float bayWidth = 0.15f;
    forEachDecor([&](const DecorItem& item, float ox, float oz) {
        if (item.kind == DecorKind::Shrub || item.kind == DecorKind::Planter ||
            item.kind == DecorKind::Bench || item.kind == DecorKind::Bin)
            return; // Furniture is drawn in the next pass.
        const float x = ox + (item.x1 + item.x2) / 2.0f;
        const float z = oz + (item.z1 + item.z2) / 2.0f;
        const float w = std::abs(item.x2 - item.x1);
        const float d = std::abs(item.z2 - item.z1);
        float y = 0.16f;
        Color surface = walkway;
        switch (item.kind) {
        case DecorKind::Garden: y = 0.18f; surface = grassPatch; break;
        case DecorKind::ServicePad: y = 0.065f; surface = servicePad; break;
        case DecorKind::ParkingPad: y = 0.065f; surface = parkingPad; break;
        case DecorKind::Plaza: y = 0.16f; surface = plaza; break;
        default: break;
        }
        drawSurface(renderer, x, z, w, d, y, surface);
        if (item.kind == DecorKind::ParkingPad) {
            // Bay separators lie along the pad's longer side, spaced across
            // the shorter side at 2.6 m so stalls read at any pad size.
            const bool spacedAlongX = w <= d;
            const float crossCount = spacedAlongX ? w : d;
            const float lineLength = (spacedAlongX ? d : w) - 1.0f;
            const int lines = static_cast<int>(std::floor(crossCount / baySpacing));
            const float step = crossCount / (lines + 1);
            for (int i = 1; i <= lines; ++i) {
                const float offset = i * step - crossCount / 2.0f;
                if (spacedAlongX)
                    drawSurface(renderer, x + offset, z, bayWidth, lineLength, 0.075f, bayLine);
                else
                    drawSurface(renderer, x, z + offset, lineLength, bayWidth, 0.075f, bayLine);
            }
        }
    });

    // Street furniture: shrubs reuse the 8-sided cylinder prism (a dark base
    // and lighter crown, colors swapped for lightCrown), planters, benches
    // and bins reuse the unit cube at their catalogue sizes.
    const Color shrubDark = {63.0f/255.0f, 124.0f/255.0f, 73.0f/255.0f};
    const Color shrubLight = {79.0f/255.0f, 153.0f/255.0f, 89.0f/255.0f};
    const Color timber = {124.0f/255.0f, 82.0f/255.0f, 56.0f/255.0f};
    const Color soil = {89.0f/255.0f, 111.0f/255.0f, 59.0f/255.0f};
    const Color seatWood = {95.0f/255.0f, 74.0f/255.0f, 52.0f/255.0f};
    const Color binNavy = {46.0f/255.0f, 61.0f/255.0f, 69.0f/255.0f};
    const Color binLid = {61.0f/255.0f, 79.0f/255.0f, 92.0f/255.0f};
    forEachDecor([&](const DecorItem& item, float ox, float oz) {
        if (item.kind == DecorKind::Shrub) {
            const auto crown = [&](float diameter, float height, float base,
                                   Color color) {
                const Matrix4 model = Matrix4::translation(ox + item.x1,
                        base + height / 2.0f, oz + item.z1)
                    * Matrix4::rotation(-90.0f, {1.0f, 0.0f, 0.0f})
                    * Matrix4::scale(diameter, diameter, height);
                renderer.draw(MeshId::Cylinder, model, color);
            };
            // Two stacked prisms fill the 0-0.8 m height range.
            crown(1.4f, 0.4f, 0.0f, item.lightCrown ? shrubLight : shrubDark);
            crown(0.95f, 0.4f, 0.4f, item.lightCrown ? shrubDark : shrubLight);
        } else if (item.kind == DecorKind::Planter) {
            drawBox(renderer, ox + item.x1, 0.4f, oz + item.z1,
                    2.0f, 0.8f, 1.2f, timber);
            drawBox(renderer, ox + item.x1, 0.85f, oz + item.z1,
                    1.3f, 0.1f, 0.85f, soil);
        } else if (item.kind == DecorKind::Bench) {
            const float width = std::abs(item.x2 - item.x1);
            const float depth = std::abs(item.z2 - item.z1);
            drawBox(renderer, ox + (item.x1 + item.x2) / 2.0f, 0.275f,
                    oz + (item.z1 + item.z2) / 2.0f,
                    width, 0.55f, depth, seatWood);
        } else if (item.kind == DecorKind::Bin) {
            drawBox(renderer, ox + item.x1, 0.45f, oz + item.z1,
                    0.7f, 0.9f, 0.7f, binNavy);
            drawBox(renderer, ox + item.x1, 0.97f, oz + item.z1,
                    0.72f, 0.06f, 0.72f, binLid);
        }
    });

    // Center and stop markings use the shared GL_LINES mesh, slightly above asphalt.
    const Color yellow = {0.91f, 0.77f, 0.28f};
    const Color white = {0.95f, 0.95f, 0.95f};
    for (int i = 1; i < roadCount - 1; ++i) {
        const float center = -cityHalfSize + i * roadSpacing;
        for (int j = 0; j < roadCount - 1; ++j) {
            const float start = -cityHalfSize + j * roadSpacing + 6.0f;
            const float end = -cityHalfSize + (j + 1) * roadSpacing - 6.0f;
            const float middle = (start + end) / 2.0f;
            for (float offset : {-0.3f, 0.3f}) {
                drawRoadLine(renderer, middle, center + offset, end - start, false, yellow);
                drawRoadLine(renderer, center + offset, middle, end - start, true, yellow);
            }
            // A stop line spans the approaching lane before each intersection.
            drawRoadLine(renderer, start - 0.5f, center - 2.0f, 4.0f, true, white);
            drawRoadLine(renderer, end + 0.5f, center + 2.0f, 4.0f, true, white);
            drawRoadLine(renderer, center + 2.0f, start - 0.5f, 4.0f, false, white);
            drawRoadLine(renderer, center - 2.0f, end + 0.5f, 4.0f, false, white);
        }
    }

    // Construct fixed layouts once; each center is half its building's height.
    for (const Building& building : cityBuildings()) {
        drawBox(renderer, building.x, building.height / 2.0f, building.z,
                building.width, building.height, building.depth, building.color);
        drawBuildingRoof(renderer, building);
    }

    // Facade positions were baked and uploaded once during initialization.
    for (const FacadeBatch& batch : facadeBatches_) {
        renderer.drawStaticMesh(batch.meshId, Matrix4::identity(), batch.color);
    }

    // Starting tower: the shared cube is centered, so lift its center by
    // half its height to place its base on the ground.
    constexpr float towerHeight = CITY_TOWER_HEIGHT;
    const Matrix4 tower = Matrix4::translation(0.0f, towerHeight / 2.0f, 0.0f)
                        * Matrix4::scale(30.0f, towerHeight, 30.0f);
    renderer.draw(MeshId::Cube, tower, {0.36f, 0.48f, 0.60f});

    // Fixed-size windows on all four facades, with 3.5-unit floor spacing.
    const Color window = {0.66f, 0.86f, 0.94f};
    const int towerFloors = std::max(0, static_cast<int>(std::floor((towerHeight - 4.5f) / 3.5f)));
    for (int floor = 0; floor < towerFloors; ++floor) {
        const float y = 4.5f + floor * 3.5f + 0.6f + 1.1f;
        for (int bay = 0; bay < 9; ++bay) {
            const float along = (bay - 4) * 3.0f;
            for (float side : {-1.0f, 1.0f}) {
                drawBox(renderer, along, y, side * 15.02f, 2.0f, 2.2f, 0.02f, window);
                drawBox(renderer, side * 15.02f, y, along, 0.02f, 2.2f, 2.0f, window);
            }
        }
    }

    // Dark lobby, service entrances, and a projecting entrance canopy.
    const Color trim = {0.18f, 0.25f, 0.31f};
    drawBox(renderer, 0.0f, 1.5f, 15.02f, 3.6f, 3.0f, 0.02f, trim);
    drawBox(renderer, 0.0f, 3.45f, 15.6f, 5.6f, 0.3f, 1.2f, trim);
    drawBox(renderer, -7.5f, 1.2f, -15.02f, 1.8f, 2.4f, 0.02f, trim);
    drawBox(renderer, 15.02f, 1.2f, 0.0f, 0.02f, 2.4f, 1.6f, trim);
    drawBox(renderer, -15.02f, 1.1f, -11.3f, 0.02f, 2.2f, 1.4f, trim);

    // Four rooftop parapets and two boxes outside the landing disc.
    for (float side : {-1.0f, 1.0f}) {
        drawBox(renderer, 0.0f, towerHeight + 0.4f, side * 14.75f, 30.0f, 0.8f, 0.5f, trim);
        drawBox(renderer, side * 14.75f, towerHeight + 0.4f, 0.0f, 0.5f, 0.8f, 29.0f, trim);
    }
    drawBox(renderer, -11.0f, towerHeight + 1.25f, -11.5f, 6.0f, 2.5f, 4.0f,
            {0.54f, 0.58f, 0.63f});
    drawBox(renderer, 12.5f, towerHeight + 1.5f, 12.5f, 2.4f, 3.0f, 2.4f,
            {0.35f, 0.40f, 0.44f});

    // Layered 24-sided discs create the dark pad and yellow ring without lighting.
    drawDisc(renderer, towerHeight + 0.03f, 18.0f, {0.23f, 0.25f, 0.28f});
    drawDisc(renderer, towerHeight + 0.05f, 16.5f, {0.91f, 0.77f, 0.28f});
    drawDisc(renderer, towerHeight + 0.07f, 15.5f, {0.23f, 0.25f, 0.28f});
    const Color helipadMark = {0.95f, 0.95f, 0.95f};
    drawSurface(renderer, -2.0f, 0.0f, 1.0f, 6.0f, towerHeight + 0.09f, helipadMark);
    drawSurface(renderer, 2.0f, 0.0f, 1.0f, 6.0f, towerHeight + 0.09f, helipadMark);
    drawSurface(renderer, 0.0f, 0.0f, 3.0f, 1.0f, towerHeight + 0.09f, helipadMark);
}
