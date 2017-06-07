// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <list>
#include <numeric>
#include <libretro.h>
#include "audio_core/audio_core.h"
#include "audio_core/libretro_sink.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/settings.h"

namespace AudioCore {

// TODO: Should this be global? I think: yes.
std::list<std::vector<s16>> queue;

struct LibRetroSink::Impl {
};

LibRetroSink::LibRetroSink() : impl(std::make_unique<Impl>()) {
}

LibRetroSink::~LibRetroSink() {
}

unsigned int LibRetroSink::GetNativeSampleRate() const {
    return 41000; // We specify this.
}

std::vector<std::string> LibRetroSink::GetDeviceList() const {
    return {"LibRetro"};
}

void LibRetroSink::EnqueueSamples(const s16* samples, size_t sample_count) {
    SubmitAudioFrames(samples, sample_count);
}

size_t LibRetroSink::SamplesInQueue() const {
    return std::accumulate(queue.begin(), queue.end(),
                           static_cast<size_t>(0), [](size_t sum, const auto& buffer) {
                // Division by two because each stereo sample is made of
                // two s16.
                return sum + buffer.size() / 2;
            });
}

void LibRetroSink::SetDevice(int device_id) {
}

void LibRetroSink::SubmitAudioFrames(const int16_t *samples,
                                                size_t sample_count) {
    //audio_batch_cb(data, frames);
    queue.emplace_back(samples, samples + sample_count * 2);
}

void audio_callback() {
    u8 buffer_backing[512];
    size_t remaining_size = static_cast<size_t>(512) /
                            sizeof(s16); // Keep track of size in 16-bit increments.
    size_t max_size = remaining_size;
    u8* buffer = buffer_backing;

    while (remaining_size > 0 && !queue.empty()) {
        if (queue.front().size() <= remaining_size) {
            memcpy(buffer, queue.front().data(), queue.front().size() * sizeof(s16));
            buffer += queue.front().size() * sizeof(s16);
            remaining_size -= queue.front().size();
            queue.pop_front();
        } else {
            memcpy(buffer, queue.front().data(), remaining_size * sizeof(s16));
            buffer += remaining_size * sizeof(s16);
            queue.front().erase(queue.front().begin(),
                                queue.front().begin() + remaining_size);
            remaining_size = 0;
        }
    }

    if (remaining_size > 0) {
        memset(buffer, 0, remaining_size * sizeof(s16));
    }

    LibRetro::SubmitAudio((const int16_t*) &buffer_backing, (max_size - remaining_size) / 2);
}

void audio_set_state(bool state) {}

} // namespace AudioCore

void LibRetro::SubmitAudio(const int16_t *data,
                           size_t frames) {
    LibRetro::audio_batch_cb(data, frames);
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    LibRetro::audio_batch_cb = cb;
}
