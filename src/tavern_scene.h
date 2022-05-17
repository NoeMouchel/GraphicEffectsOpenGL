#pragma once
#include "scene.h"

// Tavern scene data (mapped on GPU)
class tavern_scene : public scene
{
public:

    //  Constructor(s) & Destructor(s)
    //  ---------------------------------

    tavern_scene(GL::cache& GLCache);
    ~tavern_scene();

protected:

    //  Protected Fuction(s)
    //  ----------------------

    void GenerateLights() override;
    void GenerateUniforms() override;
    void LoadTexture(GL::cache& GLCache) override;
};