#pragma once

#include <vector>

#include "opengl_helpers.h"

// Tavern scene data (mapped on GPU)
class scene
{
protected:

    //  Protected Fuction(s)
    //  -----------------------

    void CreateMesh(GL::cache& GLCache, const char* filepath);

    virtual void GenerateLights() {};
    virtual void GenerateUniforms() {};
    virtual void LoadTexture(GL::cache& GLCache) {};

    //  Protected Variable(s)
    //  -----------------------

    // Lights data
    std::vector<GL::light> Lights;

public:

    //  Constructor(s) & Destructor(s)
    //  ---------------------------------

    scene() = default;
    ~scene();

    //  Public Variable(s)
    //  -------------------

    // Mesh
    GLuint VAO = 0;

    GLuint MeshBuffer = 0;
    int MeshVertexCount = 0;

    vertex_descriptor MeshDesc;

    // Lights buffer
    GLuint LightsUniformBuffer = 0;
    int LightCount = 8;

    // Textures
    GLuint DiffuseTexture = 0;
    GLuint NormalTexture = 0;
    GLuint MetallicTexture = 0;
    GLuint AOTexture = 0;
    GLuint RoughnessTexture = 0;
    GLuint EmissiveTexture = 0;
    GLuint SkyboxTexture = 0;

    //  Public Fuction(s)
    //  ------------------

    // ImGui debug function to edit lights
    void InspectLights();
    void GenerateVAO();
    void DrawScene(GLenum mode = GL_TRIANGLES);

    GL::light* GetLight(const int& i)
    {
        if ((int)Lights.size() <= i) return nullptr;

        return &Lights[i];
    }
};
