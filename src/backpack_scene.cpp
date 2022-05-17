
#include <imgui.h>

#include "platform.h"

#include "color.h"

#include "backpack_scene.h"

backpack_scene::backpack_scene(GL::cache& GLCache)
{
    CreateMesh(GLCache, "media/Survival_BackPack.obj");
    GenerateLights();
    GenerateUniforms();
    LoadTexture(GLCache);
}

backpack_scene::~backpack_scene()
{
    glDeleteBuffers(1, &LightsUniformBuffer);
}

void backpack_scene::GenerateLights()
{
    this->LightCount = 3;
    this->Lights.resize(this->LightCount);

    // (Default light, standard values)
    GL::light DefaultLight = {};
    DefaultLight.Enabled = true;
    DefaultLight.Type = LIGHT_POINT;
    DefaultLight.Position = { 0.0f, 0.0f, 0.0f };
    DefaultLight.Direction = { 0.0f, -1.0f, 0.0f };
    DefaultLight.Ambient = { 0.2f, 0.2f, 0.2f };
    DefaultLight.Diffuse = { 1.0f, 1.0f, 1.0f };
    DefaultLight.Specular = { 0.0f, 0.0f, 0.0f };
    DefaultLight.Attenuation = { 0.01f, 0.1f, 0.25f };
    DefaultLight.CutOff.x = 0.91f;
    DefaultLight.CutOff.y = 0.82f;

    // Sun light
    this->Lights[0] = this->Lights[1] = this->Lights[2] = DefaultLight;

    this->Lights[0].Type = LIGHT_DIRECTIONNAL;
    this->Lights[0].Direction = { -0.5f, -1.0f, -1.0f };
    this->Lights[0].Position = { 1.f, 3.f, 1.f }; // Directional light

    this->Lights[0].Ambient = { 180.f,238.f,241.f };
    this->Lights[0].Ambient /= 255.f;

    this->Lights[0].Diffuse = { 232.f,241.f,206.f };
    this->Lights[0].Diffuse /= 255.f;

    this->Lights[0].Specular = { 80.f,80.f,80.f };
    this->Lights[0].Specular /= 255.f;

    this->Lights[0].Diffuse = Color::RGB(0x374D58);

    this->Lights[1].Position = { 0.0f, 1.0f, -1.0f };
}

void backpack_scene::GenerateUniforms()
{
    glGenBuffers(1, &LightsUniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
    glBufferData(GL_UNIFORM_BUFFER, LightCount * sizeof(GL::light), Lights.data(), GL_DYNAMIC_DRAW);
}

void backpack_scene::LoadTexture(GL::cache& GLCache)
{
    DiffuseTexture = GLCache.LoadTexture("media/Survival_BackPack_albedo.jpg", IMG_FLIP | IMG_GEN_MIPMAPS | IMG_SRGB_SPACE);
    EmissiveTexture = GLCache.LoadTexture("media/Black.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    NormalTexture = GLCache.LoadTexture("media/Survival_BackPack_normal.png", IMG_FLIP | IMG_GEN_MIPMAPS);

    std::vector<const char*> CubemapFiles = {
        "media/SkyNight/Sky_NightTime01RT.png",
        "media/SkyNight/Sky_NightTime01LF.png",
        "media/SkyNight/Sky_NightTime01TP.png",
        "media/SkyNight/Sky_NightTime01BT.png",
        "media/SkyNight/Sky_NightTime01BK.png",
        "media/SkyNight/Sky_NightTime01FT.png",
    };

    SkyboxTexture = GLCache.LoadCubemapTexture(CubemapFiles, IMG_GEN_MIPMAPS);
}