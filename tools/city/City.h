#pragma once

#include "renderer/Color.h"
#include <cstddef>
#include <vector>

class Renderer;

class City {
public:
    bool initialize(Renderer& renderer);
    void render(Renderer& renderer) const;
private:
    struct FacadeBatch {
        std::size_t meshId;
        Color color;
    };
    std::vector<FacadeBatch> facadeBatches_;
};
