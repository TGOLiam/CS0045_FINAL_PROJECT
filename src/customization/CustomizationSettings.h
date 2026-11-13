#pragma once

// Discrete helicopter sizes; customization will select one before flight.
enum class HelicopterSize {
    Small,
    Medium,
    Large
};

struct CustomizationSettings {
    HelicopterSize size = HelicopterSize::Medium;

    float scale() const
    {
        switch (size) {
        case HelicopterSize::Small: return 1.0f;
        case HelicopterSize::Medium: return 1.5f;
        case HelicopterSize::Large: return 2.0f;
        }
        return 1.0f;
    }
};
