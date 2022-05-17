
#include <imgui.h>

#include "platform.h"

#include "color.h"

#include "tavern_scene.h"

tavern_scene::tavern_scene(GL::cache& GLCache)
{
    CreateMesh(GLCache, "media/fantasy_game_inn.obj");
    GenerateLights();
    GenerateUniforms();
    LoadTexture(GLCache);
}

tavern_scene::~tavern_scene()
{
    glDeleteBuffers(1, &LightsUniformBuffer);
}

void tavern_scene::GenerateLights()
{
    this->LightCount = 6;
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
    this->Lights[0].Position = { 1.f, 3.f, 1.f }; // Directional light
    this->Lights[0].Diffuse = Color::RGB(0x374D58);

    // Candles
    GL::light CandleLight = DefaultLight;
    CandleLight.Diffuse = Color::RGB(0xFFB400);
    CandleLight.Specular = CandleLight.Diffuse;
    CandleLight.Attenuation = { 0.f, 0.f, 2.0f };

    this->Lights[1] = this->Lights[2] = this->Lights[3] = this->Lights[4] = this->Lights[5] = CandleLight;

    // Candle positions (taken from mesh data)
    this->Lights[1].Position = { -3.214370f,-0.162299f, 5.547660f }; // Candle 1
    this->Lights[2].Position = { -4.721620f,-0.162299f, 2.590890f }; // Candle 2
    this->Lights[3].Position = { -2.661010f,-0.162299f, 0.235029f }; // Candle 3
    this->Lights[4].Position = { 0.012123f, 0.352532f,-2.302700f }; // Candle 4
    this->Lights[5].Position = { 3.030360f, 0.352532f,-1.644170f }; // Candle 5
}

void tavern_scene::GenerateUniforms()
{
    glGenBuffers(1, &LightsUniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, LightsUniformBuffer);
    glBufferData(GL_UNIFORM_BUFFER, LightCount * sizeof(GL::light), Lights.data(), GL_DYNAMIC_DRAW);
}

void tavern_scene::LoadTexture(GL::cache& GLCache)
{
    DiffuseTexture = GLCache.LoadTexture("media/fantasy_game_inn_diffuse.png", IMG_FLIP | IMG_GEN_MIPMAPS | IMG_SRGB_SPACE);
    EmissiveTexture = GLCache.LoadTexture("media/fantasy_game_inn_emissive.png", IMG_FLIP | IMG_GEN_MIPMAPS);
    NormalTexture = GLCache.LoadTexture("media/fantasy_game_inn_normal.png", IMG_FLIP | IMG_GEN_MIPMAPS);

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