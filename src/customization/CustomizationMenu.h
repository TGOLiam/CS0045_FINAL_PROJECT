#pragma once

#include "CustomizationSettings.h"

class CustomizationMenu
{
private:
    CustomizationSettings settings;

public:
    void selectSize(HelicopterSize size);
    void selectColor(HelicopterColor color);

    const CustomizationSettings& getSettings() const;
};
