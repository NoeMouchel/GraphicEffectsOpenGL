#pragma once
#include "scene.h"

// Ball scene data (mapped on GPU)
class ball_scene : public scene
{
public:

    //  Constructor(s) & Destructor(s)
    //  ---------------------------------

    ball_scene(GL::cache& GLCache);
    ~ball_scene();

protected:

    //  Protected Fuction(s)
    //  ----------------------

    void GenerateLights() override;
    void GenerateUniforms() override;
    void LoadTexture(GL::cache& GLCache) override;
};