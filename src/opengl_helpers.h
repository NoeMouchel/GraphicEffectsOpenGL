#pragma once

#include "opengl_headers.h"
#include "types.h"
#include "opengl_helpers_cache.h"
#include "opengl_helpers_wireframe.h"

enum image_flags
{
    IMG_FLIP             = 1 << 0,
    IMG_FORCE_GREY       = 1 << 1,
    IMG_FORCE_GREY_ALPHA = 1 << 2,
    IMG_FORCE_RGB        = 1 << 3,
    IMG_FORCE_RGBA       = 1 << 4,
    IMG_GEN_MIPMAPS = 1 << 5,
    IMG_SRGB_SPACE  = 1 << 6,
    IMG_CUBEMAP      = 1 << 7,
};

enum GLSL_Include
{
    GLINCLUDE_PHONGLIGHT = 1<< 0,
    GLINCLUDE_SHADOW = 2 << 0,
    GLINCLUDE_KERNELS = 3 << 0,
    GLINCLUDE_PBR = 4 << 0,
};

enum lightType
{
    LIGHT_DIRECTIONNAL = 0,
    LIGHT_POINT = 1,
    LIGHT_SPOT = 2,
};

enum ShadowCastType
{
    CASTSHADOW_NONE = 0,
    CASTSHADOW_STATIC = 1,
    CASTSHADOW_DYNAMIC = 2,
};


namespace GL
{
    // Same memory layout than 'struct light' in glsl shader
    struct light
    {
        alignas(4) int Type = 0;
        alignas(4) int Enabled;
        alignas(4) int Shadow = CASTSHADOW_STATIC;
        alignas(4) int ShadowGenerated = false;

        alignas(16) v3 Position; // (world position)
        alignas(16) v3 Direction;

        alignas(16) v3 Ambient;
        alignas(16) v3 Diffuse;
        alignas(16) v3 Specular;
        alignas(16) v3 Attenuation;

        alignas(8) v2 CutOff;

    };

    // Same memory layout than 'struct material' in glsl shader
    struct alignas(16) material
    {
        v4 Ambient;
        v4 Diffuse;
        v4 Specular;
        v4 Emission;
        float Shininess;
    };

    class debug
    {
    public:
        void WireframePrepare(GLuint MeshVBO, GLsizei PositionStride, GLsizei PositionOffset, int VertexCount)
        {
            Wireframe.BindBuffer(MeshVBO, PositionStride, PositionOffset, VertexCount);
        }

        void WireframeDrawArray(GLint First, GLsizei Count, const mat4& MVP)
        {
            Wireframe.DrawArray(First, Count, MVP);
        }
        
        GL::wireframe_renderer Wireframe;
    };

    void UniformLight(GLuint Program, const char* LightUniformName, const light& Light);
    void UniformMaterial(GLuint Program, const char* MaterialUniformName, const material& Material);
    void InjectIncludes(std::vector<const char*>& Sources, const int Includes);
    GLuint CompileShader(GLenum ShaderType, const char* ShaderStr, const int Includes = 0);
    GLuint CompileShaderEx(GLenum ShaderType, int ShaderStrsCount, const char** ShaderStrs, const int Includes = 0);
    GLuint CreateProgram(const char* VSString, const char* FSString, const int Includes = 0);
    GLuint CreateProgramEx(int VSStringsCount, const char** VSStrings, int FSStringCount, const char** FSString, const int Includes = 0);
    GLuint CreateProgramEx(int VSStringsCount, const char** VSStrings, int FSStringsCount, const char** FSStrings, int GSStringsCount, const char** GSStrings, const int Includes = 0);
    GLuint CreateProgramEx(const char* VSStrings, const char* FSStrings, const char* GSStrings, const int Includes = 0);
    const char* GetShaderStructsDefinitions();
    void UploadTexture(const char* Filename, int ImageFlags = 0, int* WidthOut = nullptr, int* HeightOut = nullptr);
    void UploadCheckerboardTexture(int Width, int Height, int SquareSize);
}
