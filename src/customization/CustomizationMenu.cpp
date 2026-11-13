#include "CustomizationMenu.h"

void CustomizationMenu::selectSize(HelicopterSize size)
{
    settings.size = size;
}

void CustomizationMenu::selectColor(HelicopterColor color)
{
    settings.color = color;
}

const CustomizationSettings& CustomizationMenu::getSettings() const
{
    return settings;
}
