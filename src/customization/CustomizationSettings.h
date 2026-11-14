#pragma once

#include "../renderer/Color.h"

enum class HelicopterSize
{
    Small,
    Medium,
    Large
};

enum class HelicopterColor
{
    Red,
    Blue,
    Green,
    White
};

struct CustomizationSettings
{
    HelicopterSize size = HelicopterSize::Medium;
    HelicopterColor color = HelicopterColor::Red;

    float scale() const
    {
        switch (size)
        {
        case HelicopterSize::Small:  return 1.0f;
        case HelicopterSize::Medium: return 1.25f;
        case HelicopterSize::Large:  return 1.5f;
        }

        return 1.25f;
    }

    float flightSpeedMultiplier() const
    {
        switch (size)
        {
        case HelicopterSize::Small:  return 3.0f;
        case HelicopterSize::Medium: return 2.0f;
        case HelicopterSize::Large:  return 1.0f;
        }

        return 2.0f;
    }

    Color bodyColor() const
    {
        switch (color)
        {
        case HelicopterColor::Red:
            return {0.8f, 0.15f, 0.12f};

        case HelicopterColor::Blue:
            return {0.12f, 0.35f, 0.85f};

        case HelicopterColor::Green:
            return {0.12f, 0.60f, 0.25f};

        case HelicopterColor::White:
            return {0.85f, 0.85f, 0.85f};
        }

        return {0.8f, 0.15f, 0.12f};
    }
};
