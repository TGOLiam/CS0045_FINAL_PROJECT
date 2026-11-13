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
    return true;
}

void City::render(Renderer& renderer) const
{
    const Matrix4 world = Matrix4::identity();
    for (const Batch& batch : batches_) {
        renderer.drawStaticMesh(batch.meshId, world, batch.color);
    }
}
