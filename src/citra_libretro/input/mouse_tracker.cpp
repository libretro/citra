// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cmath>

#include <glad/glad.h>

#include "citra_libretro/core_settings.h"
#include "citra_libretro/environment.h"
#include "citra_libretro/input/mouse_tracker.h"

#include "video_core/renderer_opengl/gl_shader_util.h"

namespace LibRetro {

namespace Input {

MouseTracker::MouseTracker() {
    // Could potentially also use Citra's built-in shaders, if they can be
    //  wrangled to cooperate.
    const GLchar* vertex = R"(
        in vec2 position;

        void main()
        {
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

    const GLchar* fragment = R"(
        out vec4 color;

        void main()
        {
            color = vec4(1.0, 1.0, 1.0, 1.0);
        }
    )";

    vao.Create();
    vbo.Create();

    glBindVertexArray(vao.handle);
    glBindBuffer(GL_ARRAY_BUFFER, vbo.handle);

    shader.Create(vertex, fragment);

    auto positionVariable = (GLuint)glGetAttribLocation(shader.handle, "position");
    glEnableVertexAttribArray(positionVariable);

    glVertexAttribPointer(positionVariable, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

MouseTracker::~MouseTracker() {
    shader.Release();
    vao.Release();
    vbo.Release();
}

void MouseTracker::OnMouseMove(int deltaX, int deltaY) {
    x += deltaX;
    y += deltaY;
}

void MouseTracker::Restrict(int minX, int minY, int maxX, int maxY) {
    x = std::min(std::max(minX, x), maxX);
    y = std::min(std::max(minY, y), maxY);
}

void MouseTracker::Update(int bufferWidth, int bufferHeight,
                          Common::Rectangle<unsigned> bottomScreen) {
    // Check mouse input
    bool state =
        (bool)(LibRetro::CheckInput(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT)) ||
        (bool)(LibRetro::CheckInput(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3));

    // TODO: Provide config option for ratios here
    auto widthSpeed = (bottomScreen.GetWidth() / 20.0);
    auto heightSpeed = (bottomScreen.GetHeight() / 20.0);

    // Read in and convert pointer values to absolute values on the canvas
    auto pointerX = LibRetro::CheckInput(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X);
    auto pointerY = LibRetro::CheckInput(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y);
    auto newX = static_cast<int>((pointerX + 0x7fff) / (float)(0x7fff * 2) * bufferWidth);
    auto newY = static_cast<int>((pointerY + 0x7fff) / (float)(0x7fff * 2) * bufferHeight);

    if ((pointerX != 0 || pointerY != 0) && (newX != lastMouseX || newY != lastMouseY)) {
        // Use mouse pointer movement
        lastMouseX = newX;
        lastMouseY = newY;

        x = std::max(static_cast<int>(bottomScreen.left),
                     std::min(newX, static_cast<int>(bottomScreen.right))) -
            bottomScreen.left;
        y = std::max(static_cast<int>(bottomScreen.top),
                     std::min(newY, static_cast<int>(bottomScreen.bottom))) -
            bottomScreen.top;
    } else if (LibRetro::settings.analog_function != LibRetro::CStickFunction::CStick) {
        // Use controller movement
        float controllerX =
            ((float)LibRetro::CheckInput(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                         RETRO_DEVICE_ID_ANALOG_X) /
             INT16_MAX);
        float controllerY =
            ((float)LibRetro::CheckInput(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                         RETRO_DEVICE_ID_ANALOG_Y) /
             INT16_MAX);

        // Deadzone the controller inputs
        float smoothedX = std::abs(controllerX);
        float smoothedY = std::abs(controllerY);

        if (smoothedX < LibRetro::settings.deadzone) {
            controllerX = 0;
        }
        if (smoothedY < LibRetro::settings.deadzone) {
            controllerY = 0;
        }

        OnMouseMove(static_cast<int>(controllerX * widthSpeed),
                    static_cast<int>(controllerY * heightSpeed));
    }

    Restrict(0, 0, bottomScreen.GetWidth(), bottomScreen.GetHeight());

    // Make the coordinates 0 -> 1
    projectedX = (float)x / bottomScreen.GetWidth();
    projectedY = (float)y / bottomScreen.GetHeight();

    // Ensure that the projected position doesn't overlap outside the bottom screen framebuffer.
    // TODO: Provide config option
    renderRatio = (float)bottomScreen.GetHeight() / 30;

    // Map the mouse coord to the bottom screen's position
    projectedX = bottomScreen.left + projectedX * bottomScreen.GetWidth();
    projectedY = bottomScreen.top + projectedY * bottomScreen.GetHeight();

    isPressed = state;

    this->bottomScreen = bottomScreen;
}

void MouseTracker::Render(int bufferWidth, int bufferHeight) {
    // Convert to OpenGL coordinates
    float centerX = (projectedX / bufferWidth) * 2 - 1;
    float centerY = (projectedY / bufferHeight) * 2 - 1;

    float renderWidth = renderRatio / bufferWidth;
    float renderHeight = renderRatio / bufferHeight;

    float boundingLeft = (bottomScreen.left / (float)bufferWidth) * 2 - 1;
    float boundingTop = (bottomScreen.top / (float)bufferHeight) * 2 - 1;
    float boundingRight = (bottomScreen.right / (float)bufferWidth) * 2 - 1;
    float boundingBottom = (bottomScreen.bottom / (float)bufferHeight) * 2 - 1;

    // Calculate the size of the vertical stalk
    float verticalLeft = std::fmax(centerX - renderWidth / 5, boundingLeft);
    float verticalRight = std::fmin(centerX + renderWidth / 5, boundingRight);
    float verticalTop = -std::fmax(centerY - renderHeight, boundingTop);
    float verticalBottom = -std::fmin(centerY + renderHeight, boundingBottom);

    // Calculate the size of the horizontal stalk
    float horizontalLeft = std::fmax(centerX - renderWidth, boundingLeft);
    float horizontalRight = std::fmin(centerX + renderWidth, boundingRight);
    float horizontalTop = -std::fmax(centerY - renderHeight / 5, boundingTop);
    float horizontalBottom = -std::fmin(centerY + renderHeight / 5, boundingBottom);

    glUseProgram(shader.handle);

    glBindVertexArray(vao.handle);

    // clang-format off
    GLfloat cursor[] = {
            // | in the cursor
            verticalLeft,  verticalTop,
            verticalRight, verticalTop,
            verticalRight, verticalBottom,

            verticalLeft,  verticalTop,
            verticalRight, verticalBottom,
            verticalLeft,  verticalBottom,

            // - in the cursor
            horizontalLeft,  horizontalTop,
            horizontalRight, horizontalTop,
            horizontalRight, horizontalBottom,

            horizontalLeft,  horizontalTop,
            horizontalRight, horizontalBottom,
            horizontalLeft,  horizontalBottom
    };
    // clang-format on

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);

    glBindBuffer(GL_ARRAY_BUFFER, vbo.handle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cursor), cursor, GL_STATIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, 12);

    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_BLEND);
}

} // namespace Input

} // namespace LibRetro
