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
    Yellow,
    Black
};

struct CustomizationSettings
{
    HelicopterSize size = HelicopterSize::Medium;
    HelicopterColor color = HelicopterColor::Red;

    float getScale() const
    {
        switch (size)
        {
            case HelicopterSize::Small:
                return 1.0f;

            case HelicopterSize::Medium:
                return 1.25f;

            case HelicopterSize::Large:
                return 1.5f;
        }

        return 1.25f;
    }

    Color getColor() const
    {
        switch (color)
        {
            case HelicopterColor::Red:
                return {0.85f, 0.15f, 0.15f};

            case HelicopterColor::Blue:
                return {0.15f, 0.35f, 0.85f};

            case HelicopterColor::Green:
                return {0.15f, 0.65f, 0.25f};

            case HelicopterColor::Yellow:
                return {0.95f, 0.75f, 0.10f};

            case HelicopterColor::Black:
                return {0.12f, 0.12f, 0.12f};
        }

        return {0.85f, 0.15f, 0.15f};
    }
};
