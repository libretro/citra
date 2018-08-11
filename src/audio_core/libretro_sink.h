// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <memory>
#include "audio_core/sink.h"
#include "libretro.h"

namespace LibRetro {

static retro_audio_sample_batch_t audio_batch_cb;

void SubmitAudio(const int16_t* data, size_t frames);

} // namespace LibRetro

namespace AudioCore {

class LibRetroSink final : public Sink {
public:
    explicit LibRetroSink(std::string target_device_name);
    ~LibRetroSink() override;

    unsigned int GetNativeSampleRate() const override;

    void EnqueueSamples(const s16* samples, size_t sample_count) override;

    size_t SamplesInQueue() const override;
};

void audio_callback();

void audio_set_state(bool new_state);

std::vector<std::string> ListLibretroSinkDevices();

} // namespace AudioCore
