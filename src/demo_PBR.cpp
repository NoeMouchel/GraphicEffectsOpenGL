
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_PBR.h"
#include "ball_scene.h"
#include "wall_scene.h"

const int LIGHT_BLOCK_BINDING_POINT = 0;

#pragma region DefaultVertexShader
static const char* gVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uModelNormalMatrix;

// Varyings
out vec2 vUV;
out vec3 vPos;    // Vertex position in view-space
out vec3 vNormal; // Vertex normal in view-space
out mat3 vTBN;

void main()
{
    vUV = aUV;
    vec4 pos4 = (uModel * vec4(aPosition, 1.0));
    vPos = pos4.xyz;
    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * uView * pos4;
    
    
   vec3 T = normalize(mat3(uModel) * aTangent);
   vec3 N =  normalize(mat3(uModel)* aNormal);
   T = normalize(T - dot(T, N) * N);
   vec3 B = cross(N, T);

   vTBN = mat3(T, B, N);

})GLSL";
#pragma endregion

#pragma region DefaultFragmentShader
static const char* gFragmentShaderStr = R"GLSL(
// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;
in mat3 vTBN;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;

uniform vec3 uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform float uAmbientOcclusion;
uniform bool uUseTextures;

uniform sampler2D uAlbedoTexture;
uniform sampler2D uMetallicTexture;
uniform sampler2D uRoughnessTexture;
uniform sampler2D uAmbientOcclusionTexture;
uniform sampler2D uNormalTexture;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Shader outputs
out vec4 oColor;

Frag_PBR_material mat = Frag_PBR_material(
    vec3(0.0),
    0.5,
    0.25,
    0.0
);

void main()
{
    mat.albedo = uUseTextures ? pow(texture(uAlbedoTexture,vUV).rgb, vec3(2.2)) : uAlbedo;
    mat.metallic =  uUseTextures ? texture(uMetallicTexture,vUV).r : uMetallic;
    mat.roughness =  uUseTextures ? texture(uRoughnessTexture,vUV).r : uRoughness;
    mat.ambientOcclusion =  uUseTextures ? texture(uAmbientOcclusionTexture,vUV).r : uAmbientOcclusion;

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0,  mat.albedo , mat.metallic);

    vec3 normal;
    if(uUseTextures)
    {
        normal = texture(uNormalTexture, vUV).rgb;
        normal = normalize(normal * 2.0 - 1.0);
        normal = normalize(vTBN * normal);
    }
    else
    {
        normal = normalize(vNormal);
    }
    
    vec3 viewDir = normalize(uViewPosition - vPos);;

    // reflectance equation
    vec3 Lo = vec3(0.0);

	for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        if(uLight[i].enabled == false) continue;
        Lo += compute_PBR_lighting(uLight[i], vPos, normal, viewDir,  mat, F0);
    }

    vec3 ambient  = vec3(0.03) * mat.albedo * mat.ambientOcclusion;
    vec3 color  = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    // Apply light color
    oColor = vec4(color, 1.0);
})GLSL";
#pragma endregion

demo_PBR::demo_PBR(GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug)
{
    scenes.push_back(new wall_scene(GLCache));
    scenes.push_back(new ball_scene(GLCache));

    for (scene* scn : scenes)
    {
        scn->GenerateVAO();
    }

    // Create shader
    {
        for (int i = 0; i < (int)scenes.size(); i++)
        {
            // Assemble fragment shader strings (defines + code)
            char FragmentShaderConfig[] = "#define LIGHT_COUNT %d\n";
            snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", scenes[i]->LightCount);
            const char* FragmentShaderStrs[2] = {
                FragmentShaderConfig,
                gFragmentShaderStr,
            };

            Program.push_back(GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, GLINCLUDE_PBR));


            glUseProgram(Program[i]);
            glUseProgram(Program[i]);
            glUniform1i(glGetUniformLocation(Program[i], "uAlbedoTexture"), 0);
            glUniform1i(glGetUniformLocation(Program[i], "uMetallicTexture"), 1);
            glUniform1i(glGetUniformLocation(Program[i], "uRoughnessTexture"), 2);
            glUniform1i(glGetUniformLocation(Program[i], "uAmbientOcclusionTexture"), 3);
            glUniform1i(glGetUniformLocation(Program[i], "uNormalTexture"), 4);
            glUniformBlockBinding(Program[i], glGetUniformBlockIndex(Program[i], "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
        }
    }}

demo_PBR::~demo_PBR()
{
    // Cleanup GL
    //glDeleteVertexArrays(1, &VAO);
    while (!scenes.empty())
    {
        delete scenes.back();
        scenes.pop_back();
    }


    while (!Program.empty())
    {
        glDeleteProgram(Program.back());
        Program.pop_back();
    }
}

void demo_PBR::Update(const platform_io& IO)
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
    this->RenderTavern(ProjectionMatrix, ViewMatrix, ModelMatrix);

    // Render tavern wireframe
    if (Wireframe)
    {
        GLDebug.Wireframe.BindBuffer(scenes[currentScene]->MeshBuffer, scenes[currentScene]->MeshDesc.Stride, scenes[currentScene]->MeshDesc.PositionOffset, scenes[currentScene]->MeshVertexCount);
        GLDebug.Wireframe.DrawArray(0, scenes[currentScene]->MeshVertexCount, ProjectionMatrix * ViewMatrix * ModelMatrix);
    }
    
    // Display debug UI
    this->DisplayDebugUI();
}

void demo_PBR::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_PBR", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        ImGui::SliderInt("Scene", &currentScene, 0, scenes.size() - 1);

        ImGui::Checkbox("UseTexture", &UseTexture);

        ImGui::ColorEdit3("Albedo", Albedo.e);
        ImGui::SliderFloat("Metallic", &Metallic,0.f,1.f);
        ImGui::SliderFloat("Roughness", &Roughness, 0.f, 1.f);
        ImGui::SliderFloat("AO", &AO, 0.f, 1.f);

        // Debug display
        ImGui::Checkbox("Wireframe", &Wireframe);
        if (ImGui::TreeNodeEx("Camera"))
        {
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", Camera.Position.x, Camera.Position.y, Camera.Position.z);
            ImGui::Text("Pitch: %.2f", Math::ToDegrees(Camera.Pitch));
            ImGui::Text("Yaw: %.2f", Math::ToDegrees(Camera.Yaw));
            ImGui::TreePop();
        }
        scenes[currentScene]->InspectLights();

        ImGui::TreePop();
    }
}

void demo_PBR::RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
{
    glEnable(GL_DEPTH_TEST);

    // Use shader and configure its uniforms
    glUseProgram(Program[currentScene]);

    // Set uniforms
    mat4 NormalMatrix = Mat4::Transpose(Mat4::Inverse(ModelMatrix));
    glUniformMatrix4fv(glGetUniformLocation(Program[currentScene], "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program[currentScene], "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program[currentScene], "uView"), 1, GL_FALSE, ViewMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Program[currentScene], "uModelNormalMatrix"), 1, GL_FALSE, NormalMatrix.e);
    glUniform3fv(glGetUniformLocation(Program[currentScene], "uViewPosition"), 1, Camera.Position.e);

    int useText = (int)UseTexture;
    glUniform1i(glGetUniformLocation(Program[currentScene], "uUseTextures"), useText);
    glUniform3fv(glGetUniformLocation(Program[currentScene], "uAlbedo"), 1, Albedo.e);
    glUniform1f(glGetUniformLocation(Program[currentScene], "uMetallic"), Metallic);
    glUniform1f(glGetUniformLocation(Program[currentScene], "uRoughness"), Roughness);
    glUniform1f(glGetUniformLocation(Program[currentScene], "uAmbientOcclusion"), AO);

    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, scenes[currentScene]->LightsUniformBuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scenes[currentScene]->DiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, scenes[currentScene]->MetallicTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, scenes[currentScene]->RoughnessTexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, scenes[currentScene]->AOTexture);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, scenes[currentScene]->NormalTexture);
    glActiveTexture(GL_TEXTURE0); // Reset active texture just in case
    
    // Draw mesh
    scenes[currentScene]->DrawScene();
}