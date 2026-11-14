#pragma once

#include "miniaudio.h"

class Audio
{
public:
    Audio() = default;
    ~Audio();

    bool initialize();
    void shutdown();

    // Sound playback
    void playUIClick();
    void startHelicopterSound();
    void stopHelicopterSound();

    // Volume controls
    void setMasterVolume(float volume);
    void setUIVolume(float volume);
    void setHelicopterVolume(float volume);

    // Mute control
    void setMuted(bool muted);

    // Get current settings
    float getMasterVolume() const;
    float getUIVolume() const;
    float getHelicopterVolume() const;
    bool isMuted() const;

private:
    ma_engine engine_{};

    ma_sound uiSound_{};
    ma_sound helicopterSound_{};

    bool initialized_ = false;
    bool uiSoundLoaded_ = false;
    bool helicopterSoundLoaded_ = false;
    bool muted_ = false;

    float masterVolume_ = 1.0f;
    float uiVolume_ = 0.15f;
    float helicopterVolume_ = 0.5f;

    void updateVolumes();
};
