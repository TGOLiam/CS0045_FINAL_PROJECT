#include "Audio.h"

void Audio::setMasterEnabled(bool enabled)
{
    masterEnabled = enabled;
}

void Audio::setEngineEnabled(bool enabled)
{
    engineEnabled = enabled;
}

void Audio::setRotorEnabled(bool enabled)
{
    rotorEnabled = enabled;
}

bool Audio::isMasterEnabled() const
{
    return masterEnabled;
}

bool Audio::isEngineEnabled() const
{
    return engineEnabled;
}

bool Audio::isRotorEnabled() const
{
    return rotorEnabled;
}

bool Audio::canPlayEngine() const
{
    return masterEnabled && engineEnabled;
}

bool Audio::canPlayRotor() const
{
    return masterEnabled && rotorEnabled;
}