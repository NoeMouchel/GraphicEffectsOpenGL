#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "backpack_scene.h"

class demo_instancing : public demo
{
public:
    demo_instancing(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_instancing();
    virtual void Update(const platform_io& IO);

    void Render(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO = 0;

    backpack_scene scene;

    float elapsedTime = 0.f;
    bool Wireframe = false;
};
