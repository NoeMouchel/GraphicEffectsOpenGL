#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

class demo_skybox : public demo
{
public:
    demo_skybox(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_skybox();
    virtual void Update(const platform_io& IO);

    void RenderSkybox(const mat4& ProjectionMatrix, const mat4& ViewMatrix);
    void RenderScene(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint SkyboxProgram = 0;
    GLuint VAO = 0;
    GLuint SkyboxVAO = 0;

    tavern_scene Scene;

    bool Wireframe = false;
    int Mode = 0;
};
