#pragma once

class Audio
{
private:
    bool masterEnabled = true;
    bool engineEnabled = true;
    bool rotorEnabled = true;

public:
    void setMasterEnabled(bool enabled);
    void setEngineEnabled(bool enabled);
    void setRotorEnabled(bool enabled);

    bool isMasterEnabled() const;
    bool isEngineEnabled() const;
    bool isRotorEnabled() const;

    bool canPlayEngine() const;
    bool canPlayRotor() const;
};