#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "scene.h"

class demo_PBR : public demo
{
public:
    demo_PBR(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_PBR();
    virtual void Update(const platform_io& IO);

    void RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    int currentScene = 0;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo

    std::vector<GLuint> Program;
    std::vector<scene*> scenes;

    //GLuint Program = 0;
    //GLuint VAO = 0;

    //ball_scene Ball;

    bool Wireframe = false;

    v3 Albedo = { 1.0f,1.0f,1.0f };
    float Metallic = 0.5f;
    float Roughness = 0.75f;
    float AO = 0.5f;
    bool UseTexture;
};