#pragma once

#include "demo.h"
#include "types.h"

#include "opengl_headers.h"

#include "camera.h"
#include "tavern_scene.h"

class demo_hdr : public demo
{
public:
    demo_hdr(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_hdr();
    virtual void Update(const platform_io& IO);

    void DisplayDebugUI();
    void RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix);

private:

    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint Texture = 0;

    GLuint hdrProgram = 0;
    GLuint hdrFBO = 0;
    GLuint colorBuffer = 0;
    GLuint rboDepth = 0;
    unsigned int HdrFramebufferWidth = 500, HdrFramebufferHeight = 500;

    bool inverseNormal = false;
    bool hdr = true;
    float exposure = 5.0;
    bool Wireframe = false;

    GLuint VAO = 0;
    GLuint VertexBuffer = 0;
    int VertexCount = 0;

    tavern_scene TavernScene;
};