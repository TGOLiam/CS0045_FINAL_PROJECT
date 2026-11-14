#define MINIAUDIO_IMPLEMENTATION
#include "Audio.h"
#include <iostream>
#include <algorithm>

Audio::~Audio()
{
    shutdown();
}

bool Audio::initialize()
{
    if (initialized_)
        return true;

    ma_result result = ma_engine_init(nullptr, &engine_);

    if (result != MA_SUCCESS)
    {
        std::cerr << "Audio engine initialization failed: "
                  << ma_result_description(result) << '\n';
        return false;
    }

    initialized_ = true;

    std::cout << "Audio engine initialized successfully.\n";

    // Load UI sound
    result = ma_sound_init_from_file(
        &engine_,
        "assets/audio/UI sound.wav",
        0,
        nullptr,
        nullptr,
        &uiSound_
    );

    if (result == MA_SUCCESS)
    {
        uiSoundLoaded_ = true;
        std::cout << "UI sound loaded successfully.\n";
    }
    else
    {
        std::cerr << "UI sound loading failed: "
                  << ma_result_description(result) << '\n';
    }

    // Load helicopter sound
    result = ma_sound_init_from_file(
        &engine_,
        "assets/audio/copter sound.wav",
        MA_SOUND_FLAG_STREAM,
        nullptr,
        nullptr,
        &helicopterSound_
    );

    if (result == MA_SUCCESS)
    {
        helicopterSoundLoaded_ = true;
        ma_sound_set_looping(&helicopterSound_, MA_TRUE);
        std::cout << "Helicopter sound loaded successfully.\n";
    }
    else
    {
        std::cerr << "Helicopter sound loading failed: "
                  << ma_result_description(result) << '\n';
    }

    // Load main menu music
    result = ma_sound_init_from_file(
        &engine_,
        "assets/audio/MainMusic.wav",
        MA_SOUND_FLAG_STREAM,
        nullptr,
        nullptr,
        &menuMusic_
    );

    if (result == MA_SUCCESS)
    {
        menuMusicLoaded_ = true;
        ma_sound_set_looping(&menuMusic_, MA_TRUE);
        std::cout << "Main menu music loaded successfully.\n";
    }
    else
    {
        std::cerr << "Main menu music loading failed: "
                  << ma_result_description(result) << '\n';
    }

    updateVolumes();

    return uiSoundLoaded_ || helicopterSoundLoaded_ || menuMusicLoaded_;
}

void Audio::shutdown()
{
    if (!initialized_)
        return;

    if (helicopterSoundLoaded_)
    {
        ma_sound_stop(&helicopterSound_);
        ma_sound_uninit(&helicopterSound_);
        helicopterSoundLoaded_ = false;
    }

    if (uiSoundLoaded_)
    {
        ma_sound_uninit(&uiSound_);
        uiSoundLoaded_ = false;
    }

    if (menuMusicLoaded_)
    {
        ma_sound_stop(&menuMusic_);
        ma_sound_uninit(&menuMusic_);
        menuMusicLoaded_ = false;
    }

    ma_engine_uninit(&engine_);

    initialized_ = false;
}

void Audio::playUIClick()
{
    if (!initialized_ || !uiSoundLoaded_ || muted_)
        return;

    ma_sound_stop(&uiSound_);
    ma_sound_seek_to_pcm_frame(&uiSound_, 0);
    ma_sound_start(&uiSound_);
}

void Audio::startHelicopterSound()
{
    if (!initialized_ || !helicopterSoundLoaded_ || muted_)
        return;

    if (!ma_sound_is_playing(&helicopterSound_))
        ma_sound_start(&helicopterSound_);
}

void Audio::stopHelicopterSound()
{
    if (!initialized_ || !helicopterSoundLoaded_)
        return;

    ma_sound_stop(&helicopterSound_);
    ma_sound_seek_to_pcm_frame(&helicopterSound_, 0);
}

// Main menu music playback
void Audio::startMenuMusic()
{
    if (!initialized_ || !menuMusicLoaded_ || muted_)
        return;

    if (!ma_sound_is_playing(&menuMusic_))
        ma_sound_start(&menuMusic_);
}

void Audio::stopMenuMusic()
{
    if (!initialized_ || !menuMusicLoaded_)
        return;

    ma_sound_stop(&menuMusic_);
    ma_sound_seek_to_pcm_frame(&menuMusic_, 0);
}

void Audio::setMasterVolume(float volume)
{
    masterVolume_ = std::clamp(volume, 0.0f, 1.0f);
    updateVolumes();
}

void Audio::setUIVolume(float volume)
{
    uiVolume_ = std::clamp(volume, 0.0f, 1.0f);
    updateVolumes();
}

void Audio::setHelicopterVolume(float volume)
{
    helicopterVolume_ = std::clamp(volume, 0.0f, 1.0f);
    updateVolumes();
}

void Audio::setMuted(bool muted)
{
    muted_ = muted;
    updateVolumes();
}

void Audio::updateVolumes()
{
    if (!initialized_)
        return;

    const float master = muted_ ? 0.0f : masterVolume_;

    if (uiSoundLoaded_)
        ma_sound_set_volume(&uiSound_, master * uiVolume_);

    if (helicopterSoundLoaded_)
        ma_sound_set_volume(
            &helicopterSound_,
            master * helicopterVolume_
        );

    if (menuMusicLoaded_)
        ma_sound_set_volume(
            &menuMusic_,
            master * menuMusicVolume_
        );
}

float Audio::getMasterVolume() const
{
    return masterVolume_;
}

float Audio::getUIVolume() const
{
    return uiVolume_;
}

float Audio::getHelicopterVolume() const
{
    return helicopterVolume_;
}

bool Audio::isMuted() const
{
    return muted_;
}
