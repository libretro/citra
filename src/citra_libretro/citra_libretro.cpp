// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <list>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <numeric>

#include "libretro.h"
#include "glad/glad.h"

#include "audio_core/libretro_sink.h"
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/loader/loader.h"
#include "core/settings.h"
#include "input_common/libretro/libretro.h"
#include "input_common/main.h"
#include "video_core/video_core.h"
#include "video_core/renderer_opengl/renderer_opengl.h"
#include "citra_libretro.h"
#include "environment.h"

// TODO: Find a better place for this junk.
Log::Filter log_filter (Log::Level::Info);
Core::System& system_core {Core::System::GetInstance()};
std::unique_ptr<EmuWindow_LibRetro> emu_window;

struct retro_hw_render_callback hw_render;

std::string file_path;

void retro_init() {
    LOG_DEBUG(Frontend, "Initialising core...");
    Log::SetFilter(&log_filter);

    InputCommon::Init();
}

void retro_deinit() {
    LOG_DEBUG(Frontend, "Shutting down core...");
    if (system_core.IsPoweredOn()) {
        system_core.Shutdown();
    }
}

unsigned retro_api_version() {
    return RETRO_API_VERSION;
}

void LibRetro::OnConfigureEnvironment() {
    static const retro_variable values[] =
    {
            { "citra_use_cpu_jit", "Enable Dynarmic JIT; enabled|disabled" },
            { "citra_use_hw_renderer", "Enable hardware renderer; enabled|disabled" },
            { "citra_use_shader_jit", "Enable shader JIT; enabled|disabled" },
            { "citra_resolution_factor", "Resolution scale factor; 1x (Native)|2x|3x|4x|5x|6x|7x|8x|9x|10x" },
            { "citra_layout_option", "Screen layout positioning; Default Top-Bottom Screen|Single Screen Only|Large Screen, Small Screen" },
            { "citra_swap_screen", "Prominent 3DS screen; Top|Bottom" },
            { "citra_limit_framerate", "Enable frame limiter; enabled|disabled" },
            { "citra_audio_stretching", "Enable audio stretching; enabled|disabled" },
            { "citra_use_virtual_sd", "Enable virtual SD card; enabled|disabled" },
            { "citra_is_new_3ds", "3DS system model; Old 3DS|New 3DS" },
            { "citra_region_value", "3DS system region; Auto|Japan|USA|Europe|Australia|China|Korea|Taiwan" },
            { "citra_use_gdbstub", "Enable GDB stub; disabled|enabled" },
            { NULL, NULL }
    };

    LibRetro::SetVariables(values);

    static const struct retro_controller_description controllers[] = {
            { "Nintendo 3DS", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0) },
    };

    static const struct retro_controller_info ports[] = {
            { controllers, 1 },
            { NULL, 0 },
    };

    LibRetro::SetControllerInfo(ports);
}

uintptr_t LibRetro::GetFramebuffer() {
    return hw_render.get_current_framebuffer();
}

/**
 * Updates Citra's settings with Libretro's.
 */
void UpdateSettings(bool init) {
    // Check to see if we actually have any config updates to process.
    if (!init && !LibRetro::HasUpdatedConfig()) {
        return;
    }

    struct retro_input_descriptor desc[] = {
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "ZL" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "ZR" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Home" },
            { 0, 0 },
    };

    LibRetro::SetInputDescriptors(desc);

    // Some settings cannot be set by LibRetro frontends - options have to be
    // finite. Make assumptions.
    Settings::values.log_filter = "*:Info";
    Settings::values.sink_id = "libretro";

    // For our other settings, import them from LibRetro.
    // TODO: Clean up this fetching maybe?
    Settings::values.use_cpu_jit = LibRetro::FetchVariable("citra_use_cpu_jit",
                                                                  "enabled").compare("enabled") == 0;
    Settings::values.use_hw_renderer = LibRetro::FetchVariable("citra_use_hw_renderer",
                                                                  "enabled").compare("enabled") == 0;
    Settings::values.use_shader_jit = LibRetro::FetchVariable("citra_use_shader_jit",
                                                                  "enabled").compare("enabled") == 0;
    Settings::values.enable_audio_stretching = LibRetro::FetchVariable("citra_audio_stretching",
                                                                  "enabled").compare("enabled") == 0;
    Settings::values.toggle_framelimit = LibRetro::FetchVariable("citra_limit_framerate",
                                                                  "enabled").compare("enabled") == 0;
    Settings::values.use_virtual_sd = LibRetro::FetchVariable("citra_use_virtual_sd",
                                                                  "enabled").compare("enabled") == 0;
    Settings::values.is_new_3ds = LibRetro::FetchVariable("citra_is_new_3ds",
                                                                  "Old 3DS").compare("New 3DS") == 0;
    Settings::values.swap_screen = LibRetro::FetchVariable("citra_swap_screen",
                                                                  "Top").compare("Bottom") == 0;
    Settings::values.use_gdbstub = LibRetro::FetchVariable("citra_use_gdbstub",
                                                                  "disabled").compare("enabled") == 0;

    // These values are a bit more hard to define, unfortunately.
    auto scaling = LibRetro::FetchVariable("citra_resolution_factor", "1x (Native)");
    auto endOfScale = scaling.find('x'); // All before 'x' in "_x ...", e.g "1x (Native)"
    if (endOfScale == std::string::npos) {
        LOG_ERROR(Frontend, "Failed to parse resolution scale!");
        Settings::values.resolution_factor = 1;
    } else {
        int scale = stoi(scaling.substr(0, endOfScale));
        Settings::values.resolution_factor = scale;
    }

    auto layout = LibRetro::FetchVariable("citra_layout_option", "Default Top-Bottom Screen");

    // TODO: Could this be reduced to a list of strings?
    if (layout.compare("Default Top-Bottom Screen") == 0) {
        Settings::values.layout_option = Settings::LayoutOption::Default;
    } else if (layout.compare("Single Screen Only") == 0) {
        Settings::values.layout_option = Settings::LayoutOption::SingleScreen;
    } else if (layout.compare("Large Screen, Small Screen") == 0) {
        Settings::values.layout_option = Settings::LayoutOption::LargeScreen;
    } else {
        LOG_ERROR(Frontend, "Unknown layout type: %s.", layout.c_str());
        Settings::values.layout_option = Settings::LayoutOption::Default;
    }

    // TODO: Region

    // Hardcode buttons to bind to libretro - it is entirely redundant to have
    //  two methods of rebinding controls.
    // Citra: A = RETRO_DEVICE_ID_JOYPAD_A (8)
    Settings::values.buttons[0] = "button:8,joystick:0,engine:libretro";
    // Citra: B = RETRO_DEVICE_ID_JOYPAD_B (0)
    Settings::values.buttons[1] = "button:0,joystick:0,engine:libretro";
    // Citra: X = RETRO_DEVICE_ID_JOYPAD_X (9)
    Settings::values.buttons[2] = "button:9,joystick:0,engine:libretro";
    // Citra: Y = RETRO_DEVICE_ID_JOYPAD_Y (1)
    Settings::values.buttons[3] = "button:1,joystick:0,engine:libretro";
    // Citra: UP = RETRO_DEVICE_ID_JOYPAD_UP (4)
    Settings::values.buttons[4] = "button:4,joystick:0,engine:libretro";
    // Citra: DOWN = RETRO_DEVICE_ID_JOYPAD_DOWN (5)
    Settings::values.buttons[5] = "button:5,joystick:0,engine:libretro";
    // Citra: LEFT = RETRO_DEVICE_ID_JOYPAD_LEFT (6)
    Settings::values.buttons[6] = "button:6,joystick:0,engine:libretro";
    // Citra: RIGHT = RETRO_DEVICE_ID_JOYPAD_RIGHT (7)
    Settings::values.buttons[7] = "button:7,joystick:0,engine:libretro";
    // Citra: L = RETRO_DEVICE_ID_JOYPAD_L (10)
    Settings::values.buttons[8] = "button:10,joystick:0,engine:libretro";
    // Citra: R = RETRO_DEVICE_ID_JOYPAD_R (11)
    Settings::values.buttons[9] = "button:11,joystick:0,engine:libretro";
    // Citra: START = RETRO_DEVICE_ID_JOYPAD_START (3)
    Settings::values.buttons[10] = "button:3,joystick:0,engine:libretro";
    // Citra: SELECT = RETRO_DEVICE_ID_JOYPAD_SELECT (2)
    Settings::values.buttons[11] = "button:2,joystick:0,engine:libretro";
    // Citra: ZL = RETRO_DEVICE_ID_JOYPAD_L2 (12)
    Settings::values.buttons[12] = "button:12,joystick:0,engine:libretro";
    // Citra: ZR = RETRO_DEVICE_ID_JOYPAD_R2 (13)
    Settings::values.buttons[13] = "button:13,joystick:0,engine:libretro";
    // Citra: HOME = RETRO_DEVICE_ID_JOYPAD_L3 (as per above bindings) (14)
    Settings::values.buttons[14] = "button:14,joystick:0,engine:libretro";

    // Circle Pad
    Settings::values.analogs[0] = "axis:0,joystick:0,engine:libretro";
    // C-Stick
    Settings::values.analogs[1] = "axis:1,joystick:0,engine:libretro";

    // Update the framebuffer sizing, but only if there is a oGL context.
    emu_window->Prepare(!init);

    Settings::Apply();
}

/**
 * libretro callback; Called every game tick.
 */
void retro_run() {
    UpdateSettings(false);

    while(!emu_window->HasSubmittedFrame()) {
        system_core.RunLoop();
    }
}

void context_reset() {
    if (VideoCore::g_renderer == nullptr) {
        return;
    }

    if (!gladLoadGL()) {
        LOG_CRITICAL(Frontend, "Glad failed to load!");
        return;
    }

    // Recreate our renderer, so it can reset it's state.
    VideoCore::g_renderer->ShutDown();

    VideoCore::g_renderer = std::make_unique<RendererOpenGL>();
    VideoCore::g_renderer->SetWindow(emu_window.get());
    if (VideoCore::g_renderer->Init()) {
        LOG_DEBUG(Render, "initialized OK");
    } else {
        LOG_ERROR(Render, "initialization failed!");
    }

    emu_window->Prepare(true);
}

void context_destroy() {
}

void retro_reset() {
    system_core.Shutdown();
    system_core.Load(emu_window.get(), file_path);
    context_reset(); // Force the renderer to appear
}

/**
 * libretro callback; Called when a game is to be loaded.
 */
bool retro_load_game(const struct retro_game_info *info) {
    LOG_INFO(Frontend, "Starting Citra RetroArch game...");

    // TODO: RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL

    file_path = info->path;

    if (!LibRetro::SetPixelFormat(RETRO_PIXEL_FORMAT_XRGB8888)) {
        LOG_CRITICAL(Frontend, "XRGB8888 is not supported.");
        LibRetro::DisplayMessage("XRGB8888 is not supported.");
        return false;
    }

    hw_render.context_type = RETRO_HW_CONTEXT_OPENGL_CORE;
    hw_render.version_major = 3;
    hw_render.version_minor = 3;
    hw_render.context_reset = context_reset;
    hw_render.context_destroy = context_destroy;
    hw_render.cache_context = false;
    hw_render.bottom_left_origin = true;
    if (!LibRetro::SetHWRenderer(&hw_render)) {
        LOG_CRITICAL(Frontend, "OpenGL 3.3 is not supported.");
        LibRetro::DisplayMessage("OpenGL 3.3 is not supported.");
        return false;
    }

    struct retro_audio_callback audio_cb = { AudioCore::audio_callback, AudioCore::audio_set_state };
    if (!LibRetro::SetAudioCallback(&audio_cb)) {
        LOG_CRITICAL(Frontend, "Async audio is not supported.");
        LibRetro::DisplayMessage("Async audio is not supported.");
        return false;
    }

    emu_window = std::make_unique<EmuWindow_LibRetro>();

    UpdateSettings(true);

    const Core::System::ResultStatus load_result{system_core.Load(emu_window.get(), file_path)};

    switch (load_result) {
        case Core::System::ResultStatus::ErrorGetLoader:
            LOG_CRITICAL(Frontend, "Failed to obtain loader for %s!", file_path.c_str());
            LibRetro::DisplayMessage("Failed to obtain loader for specified ROM!");
            return false;
        case Core::System::ResultStatus::ErrorLoader:
            LOG_CRITICAL(Frontend, "Failed to load ROM!");
            LibRetro::DisplayMessage("Failed to load ROM!");
            return false;
        case Core::System::ResultStatus::ErrorLoader_ErrorEncrypted:
            LOG_CRITICAL(Frontend, "The game that you are trying to load must be decrypted before "
                    "being used with Citra. \n\n For more information on dumping and "
                    "decrypting games, please refer to: "
                    "https://citra-emu.org/wiki/Dumping-Game-Cartridges");
            LibRetro::DisplayMessage("The game that you are trying to load must be decrypted before "
                                             "being used with Citra. \n\n For more information on dumping and "
                                             "decrypting games, please refer to: "
                                             "https://citra-emu.org/wiki/Dumping-Game-Cartridges");
            return false;
        case Core::System::ResultStatus::ErrorLoader_ErrorInvalidFormat:
            LOG_CRITICAL(Frontend, "Error while loading ROM: The ROM format is not supported.");
            LibRetro::DisplayMessage("Error while loading ROM: The ROM format is not supported.");
            return false;
        case Core::System::ResultStatus::ErrorNotInitialized:
            LOG_CRITICAL(Frontend, "CPUCore not initialized");
            LibRetro::DisplayMessage("CPUCore not initialized");
            return false;
        case Core::System::ResultStatus::ErrorSystemMode:
            LOG_CRITICAL(Frontend, "Failed to determine system mode!");
            LibRetro::DisplayMessage("Failed to determine system mode!");
            return false;
        case Core::System::ResultStatus::ErrorVideoCore:
            LOG_CRITICAL(Frontend, "VideoCore not initialized");
            LibRetro::DisplayMessage("VideoCore not initialized");
            return false;
        case Core::System::ResultStatus::Success:
            break; // Expected case
        default:
            LOG_CRITICAL(Frontend, "Unknown error");
            LibRetro::DisplayMessage("Unknown error");
            return false;
    }

    return true;
}

void retro_unload_game() {
    LOG_DEBUG(Frontend, "Unloading game...");
    system_core.Shutdown();
}

unsigned retro_get_region() {
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) {
    return retro_load_game(info);
}

size_t retro_serialize_size() {
    return 0;
}

bool retro_serialize(void *data_, size_t size) {
    return true;
}

bool retro_unserialize(const void *data_, size_t size) {
    return true;
}

void *retro_get_memory_data(unsigned id) {
    (void) id;
    return NULL;
}

size_t retro_get_memory_size(unsigned id) {
    (void) id;
    return 0;
}

void retro_cheat_reset() {}

void retro_cheat_set(unsigned index, bool enabled, const char *code) {}
