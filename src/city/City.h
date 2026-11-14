#pragma once

#include "../renderer/Color.h"
#include "../physics/Physics.h"
#include "CityActors.h"
#include <cstddef>
#include <vector>

class Renderer;

class City {
public:
    bool initialize(Renderer& renderer,
                    const ActorConfig& actorConfig = ActorConfig{});
    void update(float deltaTime);
    void render(Renderer& renderer) const;
    const std::vector<Aabb>& collisionBounds() const { return collisionBounds_; }
    float groundHeight() const { return groundHeight_; }

private:
    struct Batch {
        std::size_t meshId;
        Color color;
    };
    std::vector<Batch> batches_;
    std::vector<Aabb> collisionBounds_;
    float groundHeight_ = 0.0f;
    CityActors actors_;
};
