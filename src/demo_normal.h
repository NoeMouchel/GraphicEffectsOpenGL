#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "backpack_scene.h"
#include "ball_scene.h"
#include "scene.h"

class demo_normal : public demo
{
public:
    demo_normal(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_normal();
    virtual void Update(const platform_io& IO);

    void Render(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);
    void DisplayDebugUI();

private:
    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    int currentScene = 0;

    // GL objects needed by this demo
    std::vector<GLuint> Program;
    GLuint Debug = 0;
    std::vector<scene*> scenes;

    float elapsedTime = 0.f;
    
    bool Rotate = false;
    float RotationSpeed = 1.0f;

    bool UseSlider = false;
    float Slider = .5f;

    bool Wireframe = false;
    bool UseNormalMap = true;
    bool UseTangentSpace = true;
    bool Orthogonize = true;
    bool ShowNormals = false;
    bool ShowHalfNormal = false;
    bool ShowTangentSpaces = false;
};
