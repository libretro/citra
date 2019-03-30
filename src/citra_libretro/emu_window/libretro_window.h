// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <glad/glad.h>

#include <memory>
#include <utility>
#include "core/frontend/emu_window.h"

#include "citra_libretro/input/mouse_tracker.h"

void ResetGLState();

class EmuWindow_LibRetro : public EmuWindow {
public:
    EmuWindow_LibRetro();
    ~EmuWindow_LibRetro();

    /// Swap buffers to display the next frame
    void SwapBuffers() override;

    /// Polls window events
    void PollEvents() override;

    /// Makes the graphics context current for the caller thread
    void MakeCurrent() override;

    /// Releases the GL context from the caller thread
    void DoneCurrent() override;

    void SetupFramebuffer() override;

    /// Prepares the window for rendering
    void UpdateLayout();

    /// Enables for deferring a renderer's initalisation.
    bool ShouldDeferRendererInit() const override;

    /// States whether a frame has been submitted. Resets after call.
    bool HasSubmittedFrame();

    /// Flags that the framebuffer should be cleared.
    bool NeedsClearing() const override;

    /// Creates state for a currently running OpenGL context.
    void CreateContext();

    /// Destroys a currently running OpenGL context.
    void DestroyContext();
    
    /// Set custom layout ratios (used if citra custom_layout setting is set to true).
    void SetCustomLayoutRatios(float screen_ratio, float top_left_ratio, float top_top_ratio, float bottom_left_ratio, float bottom_top_ratio);

private:
    /// Called when a configuration change affects the minimal size of the window
    void OnMinimalClientAreaChangeRequest(
        const std::pair<unsigned, unsigned>& minimal_size) override;

    float scale = 2;
    int width;
    int height;

    bool submittedFrame = false;

    // Hack to ensure stuff runs on the main thread
    bool doCleanFrame = false;

    // For tracking LibRetro state
    bool hasTouched = false;

    GLuint framebuffer;

    // For tracking mouse cursor
    std::unique_ptr<LibRetro::Input::MouseTracker> tracker = nullptr;

    bool enableEmulatedPointer = true;
    
    // For custom layout support. 3DS screen ratio to display width (per thousand)
    float screensRatio = 0.0;
    // For custom layout support. Top screen left position ratio to display width (per thousand)
    float topLeftRatio = 0.0;
    // For custom layout support. Top screen top position ratio to display height (per thousand)
    float topTopRatio = 0.0;
    // For custom layout support. Bottom screen left position ratio to display width (per thousand)
    float bottomLeftRatio = 0.0;
    // For custom layout support. Bottom screen top position ratio to display height (per thousand)
    float bottomTopRatio = 0.0;
};
