#pragma once

#include <array>

#include "demo.h"

#include "opengl_headers.h"

#include "camera.h"

#include "tavern_scene.h"

class demo_shadowmap : public demo
{
public:
    demo_shadowmap(GL::cache& GLCache, GL::debug& GLDebug);
    virtual ~demo_shadowmap();
    virtual void Update(const platform_io& IO);

    void DepthMapsGeneration();
    void GenerateDepthMap(const mat4& DepthMVP, const int lightIndex);
    void GenerateDepthCubeMap(const std::vector<mat4>& DepthMVP, const int lightIndex);
    void RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix, const std::vector< std::vector<mat4>>& DepthMVP);
    void DisplayDebugUI();
private:

    mat4 GetLightMVP(const GL::light* light); 
    std::vector<mat4> GetPointLightMVP(const GL::light* light);

    GL::debug& GLDebug;

    // 3d camera
    camera Camera = {};

    // GL objects needed by this demo
    GLuint Program = 0;
    GLuint VAO = 0;

    GLuint DepthMapProgram = 0;
    GLuint DepthCubeMapProgram = 0;

    std::vector<GLuint> DepthMapFBO;
    std::vector<GLuint> DepthMap;

    std::vector<GLuint> DepthCubeMapFBO;
    std::vector<GLuint> DepthCubeMap;

    std::vector<std::vector<mat4>> DepthMVP;

    tavern_scene TavernScene;

    bool Wireframe = false;
};
