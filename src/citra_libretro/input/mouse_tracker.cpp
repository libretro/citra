// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <glad/glad.h>

#include "citra_libretro/input/mouse_tracker.h"
#include "citra_libretro/core_settings.h"
#include "citra_libretro/environment.h"

#include "video_core/renderer_opengl/gl_shader_util.h"

namespace LibRetro {

namespace Input {

void MouseTracker::InitOpenGL() {
    // Could potentially also use Citra's built-in shaders, if they can be
    //  wrangled to cooperate.
    const GLchar* vertex = R"(
        #version 330 core

        in vec2 position;

        void main()
        {
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

    const GLchar* fragment = R"(
        #version 330 core

        out vec4 color;

        void main()
        {
            color = vec4(1.0, 1.0, 1.0, 1.0);
        }
    )";

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    shader = GLShader::LoadProgram(vertex, fragment);

    GLuint positionVariable = (GLuint) glGetAttribLocation(shader, "position");
    glEnableVertexAttribArray(positionVariable);

    glVertexAttribPointer(positionVariable, 2, GL_FLOAT, GL_FALSE, 0, 0);
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
                          MathUtil::Rectangle<unsigned> bottomScreen) {
    // Check mouse input
    bool state = (bool) (LibRetro::CheckInput(0, RETRO_DEVICE_MOUSE,
                                                        0, RETRO_DEVICE_ID_MOUSE_LEFT))
              || (bool) (LibRetro::CheckInput(0, RETRO_DEVICE_JOYPAD,
                                                        0, RETRO_DEVICE_ID_JOYPAD_R3));

    auto mouseX = LibRetro::CheckInput(0, RETRO_DEVICE_MOUSE,
                                                            0, RETRO_DEVICE_ID_MOUSE_X) * 2;
    auto mouseY = LibRetro::CheckInput(0, RETRO_DEVICE_MOUSE,
                                                            0, RETRO_DEVICE_ID_MOUSE_Y) * 2;
    OnMouseMove(mouseX, mouseY);

    if (LibRetro::settings.analog_function != LibRetro::CStickFunction::CStick) {
        float controllerX = ((float) LibRetro::CheckInput(0, RETRO_DEVICE_ANALOG,
                                                          RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X) /
                             INT16_MAX);
        float controllerY = ((float) LibRetro::CheckInput(0, RETRO_DEVICE_ANALOG,
                                                          RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y) /
                             INT16_MAX);

        // Deadzone the controller inputs
        if (std::abs(controllerX) < LibRetro::settings.deadzone) {
            controllerX = 0;
        }
        if (std::abs(controllerY) < LibRetro::settings.deadzone) {
            controllerY = 0;
        }

        OnMouseMove((int) (controllerX * 20), (int) (controllerY * 20));
    }

    Restrict(0, 0, bufferWidth, bufferHeight);

    // Make the coordinates 0 -> 1
    projectedX = (float) x / bufferWidth;
    projectedY = (float) y / bufferHeight;

    // Ensure that the projected position doesn't overlap outside the bottom screen framebuffer.
    // TODO: Provide config option
    renderRatio = (float) (bottomScreen.bottom - bottomScreen.top) / bufferHeight / 30;
    float renderWidth  = renderRatio * bufferWidth / 2;
    float renderHeight = renderRatio * bufferHeight / 2 * ((float) bufferWidth / bufferHeight);

    // Map the mouse coord to the bottom screen's position (with a little margin)
    projectedX = bottomScreen.left + renderWidth + projectedX
                                                   * (bottomScreen.right - bottomScreen.left - renderWidth * 2);
    projectedY = bottomScreen.top + renderHeight + projectedY
                                                   * (bottomScreen.bottom - bottomScreen.top - renderHeight * 2);


    isPressed = state;
}

void MouseTracker::Render(int bufferWidth, int bufferHeight) {
    // Convert to OpenGL coordinates
    float centerX =   (projectedX / bufferWidth)  * 2 - 1;
    float centerY = -((projectedY / bufferHeight) * 2 - 1);

    float renderWidth  = renderRatio;
    float renderHeight = renderRatio * ((float) bufferWidth / bufferHeight);

    float projectedLeft   = centerX - renderWidth;
    float projectedTop    = centerY - renderHeight;
    float projectedRight  = centerX + renderWidth;
    float projectedBottom = centerY + renderHeight;

    glUseProgram(shader);

    glBindVertexArray(vao);

    GLfloat cursor[] = {
            projectedLeft,  projectedTop,
            projectedRight, projectedTop,
            projectedRight, projectedBottom,

            projectedLeft,  projectedTop,
            projectedRight, projectedBottom,
            projectedLeft,  projectedBottom
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cursor), cursor, GL_STATIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_BLEND);
}

} // namespace Input

} // namespace LibRetro
