#include "City.h"

#include "../renderer/Renderer.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>

namespace {
template <typename T>
bool readValue(std::ifstream& file, T& value)
{
    return static_cast<bool>(file.read(reinterpret_cast<char*>(&value), sizeof(value)));
}

bool validColor(Color color)
{
    return std::isfinite(color.r) && color.r >= 0.0f && color.r <= 1.0f &&
           std::isfinite(color.g) && color.g >= 0.0f && color.g <= 1.0f &&
           std::isfinite(color.b) && color.b >= 0.0f && color.b <= 1.0f &&
           std::isfinite(color.a) && color.a >= 0.0f && color.a <= 1.0f;
}
}

bool City::initialize(Renderer& renderer)
{
    // Version 1 stores little-endian 32-bit integers and IEEE-754 floats.
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
                  "City asset requires 32-bit IEEE-754 floats");
    const std::uint32_t endianCheck = 1;
    if (*reinterpret_cast<const unsigned char*>(&endianCheck) != 1) {
        std::cerr << "City asset requires a little-endian system.\n";
        return false;
    }
    std::ifstream file("assets/city/city.mesh", std::ios::binary);
    if (!file) {
        std::cerr << "Could not open assets/city/city.mesh; run from the project root.\n";
        return false;
    }
    // Match collision boxes to the exact mesh so stale sidecars are rejected.
    std::uint64_t fingerprint = 14695981039346656037ULL;
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        for (std::streamsize i = 0; i < file.gcount(); ++i) {
            fingerprint ^= static_cast<unsigned char>(buffer[i]);
            fingerprint *= 1099511628211ULL;
        }
    }
    if (file.bad()) {
        std::cerr << "Could not read city mesh for collision verification.\n";
        return false;
    }
    file.clear();
    file.seekg(0);

    std::ifstream collision("assets/city/city.collision", std::ios::binary);
    char collisionMagic[8];
    std::uint64_t expectedFingerprint = 0;
    std::uint32_t boxCount = 0;
    float ground = 0.0f;
    if (!collision.read(collisionMagic, sizeof(collisionMagic)) ||
        std::memcmp(collisionMagic, "CITYCOL1", 8) != 0 ||
        !readValue(collision, expectedFingerprint) || expectedFingerprint != fingerprint ||
        !readValue(collision, boxCount) || boxCount == 0 || boxCount > 100000 ||
        !readValue(collision, ground) || !std::isfinite(ground)) {
        std::cerr << "Invalid or mismatched city collision asset; run make city-asset.\n";
        return false;
    }
    std::vector<Aabb> boxes;
    boxes.reserve(boxCount);
    for (std::uint32_t i = 0; i < boxCount; ++i) {
        Aabb b{};
        if (!readValue(collision,b.min.x) || !readValue(collision,b.min.y) || !readValue(collision,b.min.z) ||
            !readValue(collision,b.max.x) || !readValue(collision,b.max.y) || !readValue(collision,b.max.z) ||
            !std::isfinite(b.min.x) || !std::isfinite(b.min.y) || !std::isfinite(b.min.z) ||
            !std::isfinite(b.max.x) || !std::isfinite(b.max.y) || !std::isfinite(b.max.z) ||
            b.min.x >= b.max.x || b.min.y >= b.max.y || b.min.z >= b.max.z) {
            std::cerr << "Invalid city collision box " << i << ".\n";
            return false;
        }
        boxes.push_back(b);
    }
    if (collision.peek() != std::char_traits<char>::eof()) {
        std::cerr << "Unexpected data after city collision asset.\n";
        return false;
    }
    char magic[8];
    std::uint32_t version = 0;
    std::uint32_t batchCount = 0;
    if (!file.read(magic, sizeof(magic)) || std::memcmp(magic, "CITYMESH", 8) != 0 ||
        !readValue(file, version) || version != 1 || !readValue(file, batchCount) ||
        batchCount == 0 || batchCount > 1024) {
        std::cerr << "Invalid city asset header.\n";
        return false;
    }

    std::vector<Batch> loaded;
    for (std::uint32_t i = 0; i < batchCount; ++i) {
        std::uint32_t primitive = 0;
        std::uint32_t vertexCount = 0;
        Color color{};
        if (!readValue(file, primitive) || !readValue(file, vertexCount) ||
            !readValue(file, color.r) || !readValue(file, color.g) ||
            !readValue(file, color.b) || !readValue(file, color.a) ||
            (primitive != GL_TRIANGLES && primitive != GL_LINES && primitive != GL_POINTS) ||
            vertexCount == 0 || vertexCount > 10000000 ||
            (primitive == GL_TRIANGLES && vertexCount % 3 != 0) ||
            (primitive == GL_LINES && vertexCount % 2 != 0) || !validColor(color)) {
            std::cerr << "Invalid city asset batch " << i << ".\n";
            return false;
        }
        std::vector<float> vertices(static_cast<std::size_t>(vertexCount) * 3);
        if (!file.read(reinterpret_cast<char*>(vertices.data()),
                       static_cast<std::streamsize>(vertices.size() * sizeof(float)))) {
            std::cerr << "Truncated city asset batch " << i << ".\n";
            return false;
        }
        for (float value : vertices) {
            if (!std::isfinite(value)) {
                std::cerr << "Invalid coordinate in city asset batch " << i << ".\n";
                return false;
            }
        }
        std::size_t meshId = 0;
        if (!renderer.uploadStaticMesh(vertices, static_cast<GLenum>(primitive), meshId)) {
            std::cerr << "Could not upload city asset batch " << i << ".\n";
            return false;
        }
        loaded.push_back({meshId, color});
    }
    if (file.peek() != std::char_traits<char>::eof()) {
        std::cerr << "Unexpected data after city asset.\n";
        return false;
    }
    batches_ = std::move(loaded);
    collisionBounds_ = std::move(boxes);
    groundHeight_ = ground;
    return true;
}

void City::render(Renderer& renderer) const
{
    const Matrix4 world = Matrix4::identity();
    for (const Batch& batch : batches_) {
        renderer.drawStaticMesh(batch.meshId, world, batch.color);
    }
}
