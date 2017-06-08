// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <utility>
#include "core/frontend/emu_window.h"
#include "core/frontend/motion_emu.h"

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
    void Prepare(bool hasOGL);

    /// Enables for deferring a renderer's initalisation.
    bool ShouldDeferRendererInit() const;

    /// States whether a frame has been submitted. Resets after call.
    bool HasSubmittedFrame();

    /// Hint that the mouse coordinates should be invalidated.
    void ResetTouch();
private:
    /// Called when a configuration change affects the minimal size of the window
    void OnMinimalClientAreaChangeRequest(
            const std::pair<unsigned, unsigned>& minimal_size) override;

    float scale = 2;
    int width;
    int height;

    bool submittedFrame = false;

    // For tracking LibRetro state
    bool hasTouched = false;
};
