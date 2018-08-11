// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <list>
#include <numeric>
#include <libretro.h>
#include "audio_core/libretro_sink.h"
#include "audio_types.h"
#include "core/settings.h"

namespace AudioCore {

LibRetroSink::LibRetroSink(std::string target_device_name) {}

LibRetroSink::~LibRetroSink() {}

unsigned int LibRetroSink::GetNativeSampleRate() const {
    return native_sample_rate; // We specify this.
}

void LibRetroSink::EnqueueSamples(const s16* samples, size_t sample_count) {
    LibRetro::SubmitAudio(samples, sample_count);
}

size_t LibRetroSink::SamplesInQueue() const {
    return 0;
}

std::vector<std::string> ListLibretroSinkDevices() {
    return std::vector<std::string>{"LibRetro"};
}

} // namespace AudioCore

void LibRetro::SubmitAudio(const int16_t* data, size_t frames) {
    LibRetro::audio_batch_cb(data, frames);
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    LibRetro::audio_batch_cb = cb;
}
