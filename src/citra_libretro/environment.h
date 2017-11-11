// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstdint>
#include "citra_libretro.h"
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "libretro.h"

namespace LibRetro {

/// Calls back to LibRetro to upload a particular video frame.
/// @see retro_video_refresh_t
void UploadVideoFrame(const void* data, unsigned width, unsigned height, size_t pitch);

/// Calls back to LibRetro to poll input.
/// @see retro_input_poll_t
void PollInput();

/// Sets the environmental variables used for settings.
bool SetVariables(const retro_variable vars[]);

bool SetHWSharedContext(void);

/// Returns the LibRetro save directory, or a empty string if one doesn't exist.
std::string GetSaveDir();

/// Returns the LibRetro system directory, or a empty string if one doesn't exist.
std::string GetSystemDir();

/// Fetches a variable by key name.
std::string FetchVariable(std::string key, std::string def);

/// Displays information about the kinds of controllers that this Citra recreates.
bool SetControllerInfo(const retro_controller_info info[]);

/// Sets the framebuffer pixel format.
bool SetPixelFormat(const retro_pixel_format fmt);

/// Sets the H/W rendering context.
bool SetHWRenderer(retro_hw_render_callback* cb);

/// Sets the async audio callback.
bool SetAudioCallback(retro_audio_callback* cb);

/// Set the size of the new screen buffer.
bool SetGeometry(retro_system_av_info* cb);

/// Tells LibRetro what input buttons are labelled on the 3DS.
bool SetInputDescriptors(const retro_input_descriptor desc[]);

/// Returns the current status of a input.
int16_t CheckInput(unsigned port, unsigned device, unsigned index, unsigned id);

/// Called when the emulator environment is ready to be configured.
void OnConfigureEnvironment();

/// Submits audio frames to LibRetro.
/// @see retro_audio_sample_batch_t
void SubmitAudio(const int16_t *data, size_t frames);

/// Checks to see if the frontend configuration has been updated.
bool HasUpdatedConfig();

/// Returns the current framebuffer.
uintptr_t GetFramebuffer();

/// Tells the frontend that we are done.
bool Shutdown();

/// Displays the specified message to the screen.
bool DisplayMessage(const char* sg);

} // namespace LibRetro
