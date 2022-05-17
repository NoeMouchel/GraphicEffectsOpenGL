
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_skybox.h"

const int LIGHT_BLOCK_BINDING_POINT = 0;

#pragma region DefaultVertexShader
static const char* gVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uModelNormalMatrix;

// Varyings
out vec2 vUV;
out vec3 vPos;    // Vertex position in view-space
out vec3 vNormal; // Vertex normal in view-space

void main()
{
    vUV = aUV;
    vec4 pos4 = (uModel * vec4(aPosition, 1.0));
    vPos = pos4.xyz / pos4.w;
    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * uView * pos4;
})GLSL";
#pragma endregion

#pragma region DefaultFragmentShader
static const char* gFragmentShaderStr = R"GLSL(
// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;
uniform int uMode;

uniform sampler2D uDiffuseTexture;
uniform sampler2D uEmissiveTexture;
uniform samplerCube uSkyboxCubemap;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Shader outputs
out vec4 oColor;

light_shade_result get_lights_shading()
{
    light_shade_result lightResult = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        light_shade_result light = light_shade(uLight[i], gDefaultMaterial.shininess, uViewPosition, vPos, normalize(vNormal));
        lightResult.ambient  += light.ambient;
        lightResult.diffuse  += light.diffuse;
        lightResult.specular += light.specular;
    }
    return lightResult;
}

vec4 GetReflectionColor()
{
    vec3 I = normalize(vPos - uViewPosition);
    vec3 R = reflect(I, normalize(vNormal));
    return vec4(texture(uSkyboxCubemap, R).rgb, 1.0);
}

vec4 GetRefractionColor()
{
    float ratio = 1.00 / 1.52;
    vec3 I = normalize(vPos - uViewPosition);
    vec3 R = refract(I, normalize(vNormal), ratio);
    return vec4(texture(uSkyboxCubemap, R).rgb, 1.0);
}

void main()
{
    if (uMode <= 0)
    {
        // Compute phong shading
        light_shade_result lightResult = get_lights_shading();
    

        vec3 diffuseColor  = gDefaultMaterial.diffuse * lightResult.diffuse * texture(uDiffuseTexture, vUV).rgb;
        vec3 ambientColor  = gDefaultMaterial.ambient * lightResult.ambient* texture(uDiffuseTexture, vUV).rgb;
        vec3 specularColor = gDefaultMaterial.specular * lightResult.specular;
        vec3 emissiveColor = gDefaultMaterial.emission + texture(uEmissiveTexture, vUV).rgb;
    
        // Apply light color
        oColor = vec4((ambientColor + diffuseColor + specularColor + emissiveColor), 1.0);
    }
    else if (uMode == 1)
    {
        oColor = GetReflectionColor();
    }
    else
    {
        oColor = GetRefractionColor();
    }
})GLSL";
#pragma endregion


#pragma region SkyboxVertexShader
static const char* gSkyboxVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uView;

// Varyings
out vec3 vUV;

void main()
{
    vUV = aPosition;

    vec4 pos = uProjection * uView * vec4(aPosition, 1.0);
    gl_Position = pos.xyww;
})GLSL";
#pragma endregion

#pragma region SkyboxFragmentShader

static const char* gSkyboxFragmentShaderStr = R"GLSL(
out vec4 oFragColor;

in vec3 vUV;

uniform samplerCube uSkyboxCubemap;

void main()
{    
    oFragColor = texture(uSkyboxCubemap, vUV);
}
)GLSL";
#pragma endregion

float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};


enum modes
{
    MODE_NORMAL = 0,
    MODE_REFLECTION = 1,
    MODE_REFRACTION = 2,
    COUNT = 3
};

demo_skybox::demo_skybox(GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug), Scene(GLCache)
{
    // Create shaders
    {
        // Assemble fragment shader strings (defines + code)
        char FragmentShaderConfig[] = "#define LIGHT_COUNT %d\n";
        snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", Scene.LightCount);
        const char* FragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gFragmentShaderStr,
        };

        this->Program = GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, GLINCLUDE_PHONGLIGHT);


        this->SkyboxProgram = GL::CreateProgramEx(1, &gSkyboxVertexShaderStr, 1, &gSkyboxFragmentShaderStr, false);
    }

    // Create a vertex array and bind attribs onto the vertex buffer
    {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, Scene.MeshBuffer);

        vertex_descriptor& Desc = Scene.MeshDesc;
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.PositionOffset);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.UVOffset);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.NormalOffset);

        GLuint SkyboxVBO;
        glGenVertexArrays(1, &SkyboxVAO);
        glGenBuffers(1, &SkyboxVBO);
        
        glBindVertexArray(SkyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, SkyboxVBO);
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }

    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uDiffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(Program, "uEmissiveTexture"), 1);
        glUniform1i(glGetUniformLocation(Program, "uSkyboxCubemap"), 2);
        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);

        glUseProgram(SkyboxProgram);
        glUniform1i(glGetUniformLocation(SkyboxProgram, "uSkyboxCubemap"), 0);
    }
}

demo_skybox::~demo_skybox()
{
    // Cleanup GL
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(Program);
}

void demo_skybox::Update(const platform_io& IO)
{
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;
    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    // Clear screen
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });

    // Render tavern
    this->RenderScene(ProjectionMatrix, ViewMatrix, ModelMatrix);

    // Render tavern wireframe
    if (Wireframe)
    {
        GLDebug.Wireframe.BindBuffer(Scene.MeshBuffer, Scene.MeshDesc.Stride, Scene.MeshDesc.PositionOffset, Scene.MeshVertexCount);
        GLDebug.Wireframe.DrawArray(0, Scene.MeshVertexCount, ProjectionMatrix * ViewMatrix * ModelMatrix);
    }

    // Display debug UI
    this->DisplayDebugUI();
}

void demo_skybox::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_skybox", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        ImGui::Checkbox("Wireframe", &Wireframe);

        static int selected_radio = Mode;
        if (ImGui::RadioButton("Normal Mode", &selected_radio, 0))
        {
            Mode = (int)MODE_NORMAL;
        }
        if (ImGui::RadioButton("Reflection Mode", &selected_radio, 1))
        {
            Mode = (int)MODE_REFLECTION;
        }
        if (ImGui::RadioButton("Refraction Mode", &selected_radio, 2))
        {
            Mode = (int)MODE_REFRACTION;
        }
        


        if (ImGui::TreeNodeEx("Camera"))
        {
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", Camera.Position.x, Camera.Position.y, Camera.Position.z);
            ImGui::Text("Pitch: %.2f", Math::ToDegrees(Camera.Pitch));
            ImGui::Text("Yaw: %.2f", Math::ToDegrees(Camera.Yaw));
            ImGui::TreePop();
        }
        Scene.InspectLights();

        ImGui::TreePop();
    }
}


void demo_skybox::RenderSkybox(const mat4& ProjectionMatrix, const mat4& ViewMatrix)
{
    glDepthFunc(GL_LEQUAL);
    glUseProgram(SkyboxProgram);
    glUniformMatrix4fv(glGetUniformLocation(SkyboxProgram, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);

    mat4 ViewNoTranslation = Mat4::Mat4(Mat3::Mat3(ViewMatrix));
    glUniformMatrix4fv(glGetUniformLocation(SkyboxProgram, "uView"), 1, GL_FALSE, ViewNoTranslation.e);

    glBindVertexArray(SkyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Scene.SkyboxTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthFunc(GL_LESS);
}

void demo_skybox::RenderScene(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
{
    glEnable(GL_DEPTH_TEST);

    // Use shader and configure its uniforms
    glUseProgram(Program);

    // Set uniforms
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(Program, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program, "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniform3fv(glGetUniformLocation(Program, "uViewPosition"), 1, Camera.Position.e);

    glUniform1iv(glGetUniformLocation(Program, "uMode"), 1, &Mode);
    
    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, Scene.LightsUniformBuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Scene.DiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, Scene.EmissiveTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Scene.SkyboxTexture);
    glActiveTexture(GL_TEXTURE0); // Reset active texture just in case
    
    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, Scene.MeshVertexCount);


    //  Draw skybox
    RenderSkybox(ProjectionMatrix, ViewMatrix);
}
