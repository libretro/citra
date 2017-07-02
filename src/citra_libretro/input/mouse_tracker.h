// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/math_util.h"

namespace LibRetro {

namespace Input {

/// The mouse tracker provides a mechanism to handle relative mouse/joypad input
///  for a touch-screen device.
class MouseTracker {
public:
    /// Initalises OpenGL.
    void InitOpenGL();

    /// Called whenever a mouse moves.
    void OnMouseMove(int xDelta, int yDelta);

    /// Restricts the mouse cursor to a specified rectangle.
    void Restrict(int minX, int minY, int maxX, int maxY);

    /// Updates the tracker.
    void Update(int bufferWidth, int bufferHeight, MathUtil::Rectangle<unsigned> bottomScreen);

    /// Renders the cursor to the screen.
    void Render(int bufferWidth, int bufferHeight);

    /// If the touchscreen is being pressed.
    bool IsPressed() {
        return isPressed;
    }

    /// Get the pressed position, relative to the framebuffer.
    std::pair<unsigned, unsigned> GetPressedPosition() {
        return std::pair<unsigned, unsigned>(projectedX, projectedY);
    }
private:
    int x;
    int y;

    float projectedX;
    float projectedY;
    float renderRatio;

    bool isPressed;

    GLuint shader;
    GLuint vbo;
    GLuint vao;
};

} // namespace Input

} // namespace LibRetro
