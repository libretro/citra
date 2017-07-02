// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include <core/settings.h>

#include "audio_core/libretro_sink.h"
#include "common/scm_rev.h"
#include "environment.h"

using namespace LibRetro;

namespace LibRetro {

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void UploadVideoFrame(const void* data, unsigned width,
                 unsigned height, size_t pitch) {
    return video_cb(data, width, height, pitch);
}

void PollInput() {
    return input_poll_cb();
}

bool SetVariables(const retro_variable vars[]) {
    return environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void *) vars);
}

bool SetControllerInfo(const retro_controller_info info[]) {
    return environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void *) info);
}

bool SetPixelFormat(const retro_pixel_format fmt) {
    return environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, (void *) &fmt);
}

bool SetHWRenderer(retro_hw_render_callback* cb) {
    return environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, cb);
}

bool SetAudioCallback(retro_audio_callback* cb) {
    return environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, cb);
}

bool SetGeometry(retro_system_av_info* cb) {
    return environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, cb);
}

bool SetInputDescriptors(const retro_input_descriptor desc[]) {
    return environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void *) desc);
}

bool HasUpdatedConfig() {
    bool updated = false;
    return environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated;
}

bool Shutdown() {
    return environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

/// Displays the specified message to the screen.
bool DisplayMessage(const char* sg) {
    retro_message msg;
    msg.msg = sg;
    msg.frames = 180;
    return environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
}

std::string FetchVariable(std::string key, std::string def) {
    struct retro_variable var = {0};
    var.key = key.c_str();
    if (!environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
        // Fetching variable failed.
        LOG_ERROR(Frontend, "Fetching variable %s failed.", key.c_str());
        return def;
    }
    return std::string(var.value);
}

int16_t CheckInput(unsigned port, unsigned device,
                    unsigned index, unsigned id) {
    return input_state_cb(port, device, index, id);
}

};

void retro_get_system_info(struct retro_system_info *info) {
    memset(info, 0, sizeof(*info));
    info->library_name = "Citra";
    info->library_version = Common::g_scm_desc;
    info->need_fullpath = true;
    info->valid_extensions = "3ds|3dsx|cia|elf";
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
    // We don't need single audio sample callbacks.
}

void retro_set_input_poll(retro_input_poll_t cb) {
    LibRetro::input_poll_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb) {
    LibRetro::video_cb = cb;
}
void retro_set_environment(retro_environment_t cb) {
    LibRetro::environ_cb = cb;
    LibRetro::OnConfigureEnvironment();
}

void retro_set_controller_port_device(unsigned port, unsigned device) {}

void retro_set_input_state(retro_input_state_t cb) {
    input_state_cb = cb;
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
    LOG_INFO(Frontend, "Av go!");

    // These are placeholders until we get control.
    info->timing.fps = 60.0;
    info->timing.sample_rate = 41000;
    info->geometry.base_width = 400;
    info->geometry.base_height = 480;
    info->geometry.max_width = 400 * 10;
    info->geometry.max_height = 480 * 10;
    info->geometry.aspect_ratio = 0;
}
