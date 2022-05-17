#pragma once
#include "scene.h"

// BackPack scene data (mapped on GPU)
class backpack_scene : public scene
{
public:

    //  Constructor(s) & Destructor(s)
    //  ---------------------------------

    backpack_scene(GL::cache& GLCache);
    ~backpack_scene();

protected:

    //  Protected Fuction(s)
    //  ----------------------

    void GenerateLights() override;
    void GenerateUniforms() override;
    void LoadTexture(GL::cache& GLCache) override;
};