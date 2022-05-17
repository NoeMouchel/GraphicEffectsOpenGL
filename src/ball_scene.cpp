
#include <imgui.h>

#include "platform.h"

#include "color.h"

#include "ball_scene.h"

ball_scene::ball_scene(GL::cache& GLCache)
{
    CreateMesh(GLCache, "media/CubeSphere.obj");
    GenerateLights();
    GenerateUniforms();
    LoadTexture(GLCache);
}

ball_scene::~ball_scene()
{
    glDeleteBuffers(1, &LightsUniformBuffer);
}

void ball_scene::GenerateLights()
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
    DefaultLight.Attenuation = { 1.0f, 0.0f, 0.0f };
    DefaultLight.CutOff.x = 0.91f;
    DefaultLight.CutOff.y = 0.82f;

    // Sun light
    this->Lights[0] = DefaultLight;
    this->Lights[0].Type = LIGHT_DIRECTIONNAL;
    this->Lights[0].Position = { 1.f, 3.f, 1.f };
    this->Lights[0].Diffuse = Color::RGB(0x374D58);

    // Candles
    GL::light PointLight = DefaultLight;
    PointLight.Diffuse = { 0.2f, 0.3f, 0.3f };
    PointLight.Specular = { 0.3f, 0.4f, 0.4f };

    this->Lights[1] = this->Lights[2] = PointLight;

    // Candle positions (taken from mesh data)
    this->Lights[1].Position = {0.0f, 0.0f, 3.0f}; 
    this->Lights[2].Position = { 3.0f, 0.0f, 0.0f };
}

void ball_scene::GenerateUniforms()
{
    glGenBuffers(1, &LightsUniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
    glBufferData(GL_UNIFORM_BUFFER, LightCount * sizeof(GL::light), Lights.data(), GL_DYNAMIC_DRAW);
}

void ball_scene::LoadTexture(GL::cache& GLCache)
{
    DiffuseTexture = GLCache.LoadTexture("media/PBR_Textures/rustediron2_basecolor.png", IMG_FLIP | IMG_GEN_MIPMAPS | IMG_SRGB_SPACE);
    NormalTexture = GLCache.LoadTexture("media/PBR_Textures/rustediron2_normal.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    MetallicTexture = GLCache.LoadTexture("media/PBR_Textures/rustediron2_metallic.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    RoughnessTexture = GLCache.LoadTexture("media/PBR_Textures/rustediron2_roughness.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    AOTexture = GLCache.LoadTexture("media/White.png", IMG_GEN_MIPMAPS);
    EmissiveTexture = GLCache.LoadTexture("media/Black.png", IMG_GEN_MIPMAPS);

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