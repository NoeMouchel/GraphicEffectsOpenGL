#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

class demo_postprocess : public demo
{
public:
    demo_postprocess(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_postprocess();
    virtual void Update(const platform_io& IO);

    void PreProcess();
    void PostProcess();
    void RenderScene(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint PostProcessProgram = 0;
    GLuint VAO = 0;

    GLuint ScreenVAO = 0;
    GLuint ScreenFBO = 0;
    GLuint ScreenRBO = 0;
    GLuint ScreenTexture = 0;

    tavern_scene TavernScene;

    bool Wireframe = false;

    bool Shake = false;
    float ShakeTime = .25f; 
    float ElapsedTimeShaking = 0.f;

    int ScreenWidth, ScreenHeight;
    int Mode = 0;
};
