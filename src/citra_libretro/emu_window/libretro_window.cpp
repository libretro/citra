// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <glad/glad.h>
#include <libretro.h>

#include "audio_core/audio_types.h"
#include "citra_libretro/citra_libretro.h"
#include "citra_libretro/environment.h"
#include "citra_libretro/input/input_factory.h"
#include "core/3ds.h"
#include "core/settings.h"
#include "video_core/renderer_opengl/gl_state.h"



/// LibRetro expects a "default" GL state.
void ResetGLState() {
    // Reset internal state.
    OpenGL::OpenGLState state{};
    state.Apply();

    // Clean up global state.
    if (!Settings::values.use_gles) {
        glLogicOp(GL_COPY);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendColor(0, 0, 0, 0);

    glDisable(GL_COLOR_LOGIC_OP);

    glDisable(GL_DITHER);

    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glActiveTexture(GL_TEXTURE0);
}

EmuWindow_LibRetro::EmuWindow_LibRetro() {}

EmuWindow_LibRetro::~EmuWindow_LibRetro() {}

void EmuWindow_LibRetro::SwapBuffers() {
    submittedFrame = true;

    auto current_state = OpenGL::OpenGLState::GetCurState();

    ResetGLState();

    if (tracker != nullptr) {
        tracker->Render(width, height);
    }

    LibRetro::UploadVideoFrame(RETRO_HW_FRAME_BUFFER_VALID, static_cast<unsigned>(width),
                               static_cast<unsigned>(height), 0);

    ResetGLState();

    current_state.Apply();
}

void EmuWindow_LibRetro::SetupFramebuffer() {
    // TODO: Expose interface in renderer_opengl to configure this in it's internal state
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(LibRetro::GetFramebuffer()));

    // glClear can be a slow path - skip clearing if we don't need to.
    if (doCleanFrame) {
        glClear(GL_COLOR_BUFFER_BIT);

        doCleanFrame = false;
    }
}

void EmuWindow_LibRetro::PollEvents() {
    LibRetro::PollInput();

    // TODO: Poll for right click for motion emu

    if (tracker != nullptr) {
        tracker->Update(width, height, GetFramebufferLayout().bottom_screen);

        if (tracker->IsPressed()) {
            auto mousePos = tracker->GetPressedPosition();

            if (hasTouched) {
                TouchMoved(mousePos.first, mousePos.second);
            } else {
                TouchPressed(mousePos.first, mousePos.second);
                hasTouched = true;
            }
        } else if (hasTouched) {
            hasTouched = false;
            TouchReleased();
        }
    }
}

void EmuWindow_LibRetro::MakeCurrent() {
    // They don't get any say in the matter - GL context is always current!
}

void EmuWindow_LibRetro::DoneCurrent() {
    // They don't get any say in the matter - GL context is always current!
}

void EmuWindow_LibRetro::OnMinimalClientAreaChangeRequest(std::pair<u32, u32> _minimal_size) {
}

void EmuWindow_LibRetro::UpdateLayout() {
    // TODO: Handle custom layouts
    // TODO: Extract this ugly thing somewhere else
    unsigned baseX;
    unsigned baseY;

    float scaling = Settings::values.resolution_factor;

    bool swapped = Settings::values.swap_screen;

    enableEmulatedPointer = true;

    switch (Settings::values.layout_option) {
    case Settings::LayoutOption::SingleScreen:
        if (swapped) { // Bottom screen visible
            baseX = Core::kScreenBottomWidth;
            baseY = Core::kScreenBottomHeight;
        } else { // Top screen visible
            baseX = Core::kScreenTopWidth;
            baseY = Core::kScreenTopHeight;
            enableEmulatedPointer = false;
        }
        baseX *= scaling;
        baseY *= scaling;
        break;
    case Settings::LayoutOption::LargeScreen:
        if (swapped) { // Bottom screen biggest
            baseX = Core::kScreenBottomWidth + Core::kScreenTopWidth / 4;
            baseY = Core::kScreenBottomHeight;
        } else { // Top screen biggest
            baseX = Core::kScreenTopWidth + Core::kScreenBottomWidth / 4;
            baseY = Core::kScreenTopHeight;
        }

        if (scaling < 4) {
            // Unfortunately, to get this aspect ratio correct (and have non-blurry 1x scaling),
            //  we have to have a pretty large buffer for the minimum ratio.
            baseX *= 4;
            baseY *= 4;
        } else {
            baseX *= scaling;
            baseY *= scaling;
        }
        break;
    case Settings::LayoutOption::SideScreen:
        baseX = Core::kScreenBottomWidth + Core::kScreenTopWidth;
        baseY = Core::kScreenTopHeight;
        baseX *= scaling;
        baseY *= scaling;
        break;
    case Settings::LayoutOption::Default:
    default:
        if (swapped) { // Bottom screen on top
            baseX = Core::kScreenBottomWidth;
        } else { // Top screen on top
            baseX = Core::kScreenTopWidth;
        }
        baseY = Core::kScreenTopHeight + Core::kScreenBottomHeight;
        baseX *= scaling;
        baseY *= scaling;
        break;
    }

    // Update Libretro with our status
    struct retro_system_av_info info {};
    info.timing.fps = 60.0;
    info.timing.sample_rate = AudioCore::native_sample_rate;
    info.geometry.aspect_ratio = (float)baseX / (float)baseY;
    info.geometry.base_width = baseX;
    info.geometry.base_height = baseY;
    info.geometry.max_width = baseX;
    info.geometry.max_height = baseY;
    if (!LibRetro::SetGeometry(&info)) {
        LOG_CRITICAL(Frontend, "Failed to update 3DS layout in frontend!");
    }

    NotifyClientAreaSizeChanged(std::pair<unsigned, unsigned>(baseX, baseY));

    width = baseX;
    height = baseY;

    UpdateCurrentFramebufferLayout(baseX, baseY);

    doCleanFrame = true;
}

bool EmuWindow_LibRetro::ShouldDeferRendererInit() {
    // Do not defer renderer init after first init, used for savestates
    if(!firstInit) return false;
    firstInit = false;

    // load_game doesn't always provide a GL context.
    return true;
}

bool EmuWindow_LibRetro::NeedsClearing() const {
    // We manage this ourselves.
    return false;
}

bool EmuWindow_LibRetro::HasSubmittedFrame() {
    bool state = submittedFrame;
    submittedFrame = false;
    return state;
}

void EmuWindow_LibRetro::CreateContext() {
    if (enableEmulatedPointer) {
        tracker = std::make_unique<LibRetro::Input::MouseTracker>();
    }

    doCleanFrame = true;
}

void EmuWindow_LibRetro::DestroyContext() {
    tracker = nullptr;
}
