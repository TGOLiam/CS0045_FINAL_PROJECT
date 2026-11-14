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
        case HelicopterSize::Medium: return 2.0f;
        case HelicopterSize::Large: return 2.5f;
        }
        return 1.0f;
    }

    // Multipliers relative to the original flight tuning: Medium is 2×,
    // Small is 1.5× Medium, and Large is 0.5× Medium.
    float flightSpeedMultiplier() const
    {
        switch (size) {
        case HelicopterSize::Small: return 3.0f;
        case HelicopterSize::Medium: return 2.0f;
        case HelicopterSize::Large: return 1.0f;
        }
        return 2.0f;
    }
};
