#pragma once
#include "scene.h"

// Wall scene data (mapped on GPU)
class wall_scene : public scene
{
public:

    //  Constructor(s) & Destructor(s)
    //  ---------------------------------

    wall_scene(GL::cache& GLCache);
    ~wall_scene();

protected:

    //  Protected Fuction(s)
    //  ----------------------

    void GenerateLights() override;
    void GenerateUniforms() override;
    void LoadTexture(GL::cache& GLCache) override;
};