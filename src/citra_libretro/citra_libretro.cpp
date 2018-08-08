// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <list>
#include <numeric>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "glad/glad.h"
#include "libretro.h"

#include "audio_core/libretro_sink.h"
#include "citra_libretro/citra_libretro.h"
#include "citra_libretro/core_settings.h"
#include "citra_libretro/environment.h"
#include "citra_libretro/libretro_logger.h"
#include "citra_libretro/input/input_factory.h"
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/memory.h"
#include "core/hle/kernel/memory.h"
#include "core/loader/loader.h"
#include "core/settings.h"
#include "video_core/renderer_opengl/renderer_opengl.h"
#include "video_core/video_core.h"

class CitraLibRetro {
public:
    CitraLibRetro() : log_filter(Log::Level::Info) {}

    Log::Filter log_filter;
    std::unique_ptr<EmuWindow_LibRetro> emu_window;
    struct retro_hw_render_callback hw_render {};
};

CitraLibRetro* emu_instance;

void retro_init() {
    emu_instance = new CitraLibRetro();
    Log::Init();
    Log::SetGlobalFilter(emu_instance->log_filter);

    // Check to see if the frontend is providing us with logging functionality
    auto callback = LibRetro::GetLoggingBackend();
    if (callback != nullptr) {
        Log::AddBackend(std::make_unique<LibRetroLogger>(callback));
    } else {
        Log::AddBackend(std::make_unique<Log::ColorConsoleBackend>());
    }

    LOG_DEBUG(Frontend, "Initialising core...");

    LibRetro::Input::Init();
}

void retro_deinit() {
    LOG_DEBUG(Frontend, "Shutting down core...");
    if (Core::System::GetInstance().IsPoweredOn()) {
        Core::System::GetInstance().Shutdown();
    }

    LibRetro::Input::Shutdown();

    delete emu_instance;

    Log::Destroy();
}

unsigned retro_api_version() {
    return RETRO_API_VERSION;
}

void LibRetro::OnConfigureEnvironment() {
    static const retro_variable values[] = {
        {"citra_use_cpu_jit", "Enable CPU JIT; enabled|disabled"},
        {"citra_use_hw_renderer", "Enable hardware renderer; enabled|disabled"},
        {"citra_use_shader_jit", "Enable shader JIT; enabled|disabled"},
        {"citra_use_hw_shaders", "Enable hardware shaders; enabled|disabled"},
        {"citra_use_acc_geo_shaders", "Enable accurate geometry shaders (only for H/W shaders); enabled|disabled"},
        {"citra_use_acc_mul", "Enable accurate shaders multiplication (only for H/W shaders); enabled|disabled"},
        {"citra_resolution_factor",
         "Resolution scale factor; 1x (Native)|2x|3x|4x|5x|6x|7x|8x|9x|10x"},
        {"citra_layout_option", "Screen layout positioning; Default Top-Bottom Screen|Single "
                                "Screen Only|Large Screen, Small Screen|Side by Side"},
        {"citra_swap_screen", "Prominent 3DS screen; Top|Bottom"},
        {"citra_analog_function",
         "Right analog function; C-Stick and Touchscreen Pointer|Touchscreen Pointer|C-Stick"},
        {"citra_deadzone", "Emulated pointer deadzone (%); 15|20|25|30|35|0|5|10"},
        {"citra_use_virtual_sd", "Enable virtual SD card; enabled|disabled"},
        {"citra_use_libretro_save_path", "Savegame location; LibRetro Default|Citra Default"},
        {"citra_is_new_3ds", "3DS system model; Old 3DS|New 3DS"},
        {"citra_region_value",
         "3DS system region; Auto|Japan|USA|Europe|Australia|China|Korea|Taiwan"},
        {"citra_use_gdbstub", "Enable GDB stub; disabled|enabled"},
        {nullptr, nullptr}};

    LibRetro::SetVariables(values);

    static const struct retro_controller_description controllers[] = {
        {"Nintendo 3DS", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)},
    };

    static const struct retro_controller_info ports[] = {
        {controllers, 1},
        {nullptr, 0},
    };

    LibRetro::SetControllerInfo(ports);
}

uintptr_t LibRetro::GetFramebuffer() {
    return emu_instance->hw_render.get_current_framebuffer();
}

/**
 * Updates Citra's settings with Libretro's.
 */
void UpdateSettings() {
    struct retro_input_descriptor desc[] = {
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "ZL"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "ZR"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "Home"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "Touch Screen Touch"},
        {0, 0},
    };

    LibRetro::SetInputDescriptors(desc);

    // Some settings cannot be set by LibRetro frontends - options have to be
    // finite. Make assumptions.
    Settings::values.log_filter = "*:Info";
    Settings::values.sink_id = "libretro";

    // We don't need these, as this is the frontend's responsibility.
    Settings::values.enable_audio_stretching = false;
    Settings::values.use_frame_limit = false;
    Settings::values.frame_limit = 100;

    // For our other settings, import them from LibRetro.
    Settings::values.use_cpu_jit =
        LibRetro::FetchVariable("citra_use_cpu_jit", "enabled") == "enabled";
    Settings::values.use_hw_renderer =
        LibRetro::FetchVariable("citra_use_hw_renderer", "enabled") == "enabled";
    Settings::values.use_hw_shader =
            LibRetro::FetchVariable("citra_use_hw_shaders", "enabled") == "enabled";
    Settings::values.use_shader_jit =
        LibRetro::FetchVariable("citra_use_shader_jit", "enabled") == "enabled";
    Settings::values.shaders_accurate_gs =
            LibRetro::FetchVariable("citra_use_acc_geo_shaders", "enabled") == "enabled";
    Settings::values.shaders_accurate_mul =
            LibRetro::FetchVariable("citra_use_acc_mul", "enabled") == "enabled";
    Settings::values.use_virtual_sd =
        LibRetro::FetchVariable("citra_use_virtual_sd", "enabled") == "enabled";
    Settings::values.is_new_3ds =
        LibRetro::FetchVariable("citra_is_new_3ds", "Old 3DS") == "New 3DS";
    Settings::values.swap_screen = LibRetro::FetchVariable("citra_swap_screen", "Top") == "Bottom";
    Settings::values.use_gdbstub =
        LibRetro::FetchVariable("citra_use_gdbstub", "disabled") == "enabled";

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

    if (layout == "Default Top-Bottom Screen") {
        Settings::values.layout_option = Settings::LayoutOption::Default;
    } else if (layout == "Single Screen Only") {
        Settings::values.layout_option = Settings::LayoutOption::SingleScreen;
    } else if (layout == "Large Screen, Small Screen") {
        Settings::values.layout_option = Settings::LayoutOption::LargeScreen;
    } else if (layout == "Side by Side") {
        Settings::values.layout_option = Settings::LayoutOption::SideScreen;
    } else {
        LOG_ERROR(Frontend, "Unknown layout type: {}.", layout);
        Settings::values.layout_option = Settings::LayoutOption::Default;
    }

    auto deadzone = LibRetro::FetchVariable("citra_deadzone", "15");
    LibRetro::settings.deadzone = (float)std::stoi(deadzone) / 100;

    auto analog_function =
        LibRetro::FetchVariable("citra_analog_function", "C-Stick and Touchscreen Pointer");

    if (analog_function == "C-Stick and Touchscreen Pointer") {
        LibRetro::settings.analog_function = LibRetro::CStickFunction::Both;
    } else if (analog_function == "C-Stick") {
        LibRetro::settings.analog_function = LibRetro::CStickFunction::CStick;
    } else if (analog_function == "Touchscreen Pointer") {
        LibRetro::settings.analog_function = LibRetro::CStickFunction::Touchscreen;
    } else {
        LOG_ERROR(Frontend, "Unknown right analog function: {}.", analog_function);
        LibRetro::settings.analog_function = LibRetro::CStickFunction::Both;
    }

    auto region = LibRetro::FetchVariable("citra_region_value", "Auto");
    std::map<std::string, int> region_values;
    region_values["Auto"] = 0;
    region_values["Japan"] = 1;
    region_values["USA"] = 2;
    region_values["Europe"] = 3;
    region_values["Australia"] = 4;
    region_values["China"] = 5;
    region_values["Korea"] = 6;
    region_values["Taiwan"] = 7;

    auto result = region_values.find(region);
    if (result == region_values.end()) {
        LOG_ERROR(Frontend, "Invalid region: {}.", region);
        Settings::values.region_value = 0;
    } else {
        Settings::values.region_value = result->second;
    }

    Settings::values.touch_device = "engine:emu_window";

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
    if (LibRetro::settings.analog_function != LibRetro::CStickFunction::Touchscreen) {
        Settings::values.analogs[1] = "axis:1,joystick:0,engine:libretro";
    } else {
        Settings::values.analogs[1] = "";
    }

    // Configure the file storage location
    auto use_libretro_saves = LibRetro::FetchVariable("citra_use_libretro_save_path",
                                                      "LibRetro Default") == "LibRetro Default";

    if (use_libretro_saves) {
        auto target_dir = LibRetro::GetSaveDir();
        if (target_dir.empty()) {
            LOG_INFO(Frontend, "No save dir provided; trying system dir...");
            target_dir = LibRetro::GetSystemDir();
        }

        if (!target_dir.empty()) {
            target_dir += "/Citra";

            // Ensure that this new dir exists
            if (!FileUtil::CreateDir(target_dir)) {
                LOG_ERROR(Frontend, "Failed to create \"{}\". Using Citra's default paths.", target_dir);
            } else {
                FileUtil::GetUserPath(D_ROOT_IDX, target_dir);
                const auto& target_dir_result = FileUtil::GetUserPath(D_USER_IDX, target_dir);
                LOG_INFO(Frontend, "User dir set to \"{}\".", target_dir_result);
            }
        }
    }

    // Update the framebuffer sizing.
    emu_instance->emu_window->UpdateLayout();

    Settings::Apply();
}

/**
 * libretro callback; Called every game tick.
 */
void retro_run() {
    // Check to see if we actually have any config updates to process.
    if (LibRetro::HasUpdatedConfig()) {
        UpdateSettings();
    }

    // We can't assume that the frontend has been nice and preserved all OpenGL settings. Reset.
    auto last_state = OpenGLState::GetCurState();
    ResetGLState();
    last_state.Apply();

    while (!emu_instance->emu_window->HasSubmittedFrame()) {
        auto result = Core::System::GetInstance().RunLoop();

        if (result != Core::System::ResultStatus::Success) {
            std::string errorContent = Core::System::GetInstance().GetStatusDetails();
            std::string msg;

            switch (result) {
            case Core::System::ResultStatus::ErrorSystemFiles:
                msg = "Citra was unable to locate a 3DS system archive: " + errorContent;
                break;
            case Core::System::ResultStatus::ErrorSharedFont:
                msg = "Citra was unable to locate the 3DS shared fonts.";
                break;
            default:
                msg = "Fatal Error encountered: " + errorContent;
                break;
            }

            LibRetro::DisplayMessage(msg.c_str());
        }
    }
}

void* load_opengl_func(const char* name) {
    return (void*)emu_instance->hw_render.get_proc_address(name);
}

void context_reset() {
    if (!Core::System::GetInstance().IsPoweredOn()) {
        LOG_CRITICAL(Frontend, "Cannot reset system core if isn't on!");
        return;
    }

    // Check to see if the frontend provides us with OpenGL symbols
    if (emu_instance->hw_render.get_proc_address != nullptr) {
        if (!gladLoadGLLoader((GLADloadproc)load_opengl_func)) {
            LOG_CRITICAL(Frontend, "Glad failed to load (frontend-provided symbols)!");
            return;
        }
    } else {
        // Else, try to load them on our own
        if (!gladLoadGL()) {
            LOG_CRITICAL(Frontend, "Glad failed to load (internal symbols)!");
            return;
        }
    }

    // Recreate our renderer, so it can reset it's state.
    if (VideoCore::g_renderer != nullptr) {
        LOG_ERROR(Frontend,
                  "Likely memory leak: context_destroy() was not called before context_reset()!");
    }

    VideoCore::g_renderer = std::make_unique<RendererOpenGL>();
    VideoCore::g_renderer->SetWindow(emu_instance->emu_window.get());
    if (VideoCore::g_renderer->Init()) {
        LOG_DEBUG(Render, "initialized OK");
    } else {
        LOG_ERROR(Render, "initialization failed!");
    }

    emu_instance->emu_window->UpdateLayout();
    emu_instance->emu_window->CreateContext();
}

void context_destroy() {
    if (VideoCore::g_renderer != nullptr) {
        VideoCore::g_renderer->ShutDown();
    }

    emu_instance->emu_window->DestroyContext();
    VideoCore::g_renderer = nullptr;
}

void retro_reset() {
    Core::System::GetInstance().Shutdown();
    Core::System::GetInstance().Load(emu_instance->emu_window.get(), LibRetro::settings.file_path);
    context_reset(); // Force the renderer to appear
}

/**
 * libretro callback; Called when a game is to be loaded.
 */
bool retro_load_game(const struct retro_game_info* info) {
    LOG_INFO(Frontend, "Starting Citra RetroArch game...");

    LibRetro::settings.file_path = info->path;

    LibRetro::SetHWSharedContext();

    if (!LibRetro::SetPixelFormat(RETRO_PIXEL_FORMAT_XRGB8888)) {
        LOG_CRITICAL(Frontend, "XRGB8888 is not supported.");
        LibRetro::DisplayMessage("XRGB8888 is not supported.");
        return false;
    }

    emu_instance->hw_render.context_type = RETRO_HW_CONTEXT_OPENGL_CORE;
    emu_instance->hw_render.version_major = 3;
    emu_instance->hw_render.version_minor = 3;
    emu_instance->hw_render.context_reset = context_reset;
    emu_instance->hw_render.context_destroy = context_destroy;
    emu_instance->hw_render.cache_context = false;
    emu_instance->hw_render.bottom_left_origin = true;
    if (!LibRetro::SetHWRenderer(&emu_instance->hw_render)) {
        LOG_CRITICAL(Frontend, "OpenGL 3.3 is not supported.");
        LibRetro::DisplayMessage("OpenGL 3.3 is not supported.");
        return false;
    }

    emu_instance->emu_window = std::make_unique<EmuWindow_LibRetro>();
    UpdateSettings();

    const Core::System::ResultStatus load_result{Core::System::GetInstance().Load(
        emu_instance->emu_window.get(), LibRetro::settings.file_path)};

    switch (load_result) {
    case Core::System::ResultStatus::ErrorGetLoader:
        LOG_CRITICAL(Frontend, "Failed to obtain loader for {}!",
                     LibRetro::settings.file_path);
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
    Core::System::GetInstance().Shutdown();
}

unsigned retro_get_region() {
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info* info,
                             size_t num_info) {
    return retro_load_game(info);
}

size_t retro_serialize_size() {
    return 0;
}

bool retro_serialize(void* data_, size_t size) {
    return true;
}

bool retro_unserialize(const void* data_, size_t size) {
    return true;
}

void* retro_get_memory_data(unsigned id) {
    if ( id == RETRO_MEMORY_SYSTEM_RAM )
        return Kernel::memory_regions[0].linear_heap_memory->data() ;

    return NULL;
}

size_t retro_get_memory_size(unsigned id) {
    if ( id == RETRO_MEMORY_SYSTEM_RAM )
        return Kernel::memory_regions[0].size ;

    return 0;
}

void retro_cheat_reset() {}

void retro_cheat_set(unsigned index, bool enabled, const char* code) {}
