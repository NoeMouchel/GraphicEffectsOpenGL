
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_normal.h"

#include "backpack_scene.h"
#include "ball_scene.h"
#include "wall_scene.h"

const int LIGHT_BLOCK_BINDING_POINT = 0;

static const char* gVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uModelNormalMatrix;
uniform bool uOrthogonize;

// Varyings
out vec2 vUV;
out vec3 vPos;    // Vertex position in view-space
out vec3 vNormal; // Vertex normal in view-space
out mat3 vTBN;

void main()
{
    vUV = aUV;
    vec4 pos4 = (uModel * vec4(aPosition, 1.0));
    //vPos = pos4.xyz / pos4.w;
    vPos = pos4.xyz;

    vNormal = (uModelNormalMatrix * vec4(aNormal, 0.0)).xyz;
    gl_Position = uProjection * uView * pos4;

    if (uOrthogonize)
    {
        vec3 T = normalize(mat3(uModel) * aTangent);
        vec3 N =  normalize(mat3(uModel)* aNormal);
        T = normalize(T - dot(T, N) * N);
        vec3 B = cross(N, T);

        vTBN = mat3(T, B, N); 
    }
    else
    {
        vec3 T = normalize(vec3(uModel * vec4(aTangent, 0.0)));
        vec3 N =  normalize(vec3(uModel * vec4(aNormal, 0.0)));
        vec3 B = normalize(vec3(uModel * vec4(aBitangent, 0.0)));

        vTBN = mat3(T, B, N); 
    }
})GLSL";

static const char* gFragmentShaderStr = R"GLSL(
// Varyings
in vec2 vUV;
in vec3 vPos;
in vec3 vNormal;
in mat3 vTBN;

// Uniforms
uniform mat4 uProjection;
uniform vec3 uViewPosition;
uniform bool uShowNormal;
uniform bool uHasNormal;
uniform bool uUseTangentSpace;
uniform bool uUseSlider;
uniform float uSliderValue;
uniform bool uShowHalfNormal;

uniform sampler2D uDiffuseTexture;
uniform sampler2D uEmissiveTexture;
uniform sampler2D uNormalTexture;


vec3 Normal;

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
        light currlight =  uLight[i];
        light_shade_result light = light_shade(currlight, gDefaultMaterial.shininess, uViewPosition,vPos, normalize(Normal));
        lightResult.ambient  += light.ambient;
        lightResult.diffuse    += light.diffuse;
        lightResult.specular += light.specular;
    }
    return lightResult;
}

void main()
{
    bool useNormal = uHasNormal && (uUseSlider ? false : true);

    bool showNormal = uShowNormal;

    if(uHasNormal && uUseSlider)
    {
        bool showHalfNormal = uShowHalfNormal && !showNormal;
        if(vPos.x < uSliderValue + 0.005 && vPos.x > uSliderValue - 0.005
            ||(showHalfNormal && (vPos.y < 0.005 && vPos.y > -0.005)))
        {
            oColor = vec4(1.0);
            return;
        }
        useNormal = vPos.x > uSliderValue;
        
        
        if(showHalfNormal)
        {
            if(vPos.y < 0.0f)
            {
                showNormal = true;
            }
        }
    }
    
    if(useNormal)
    {
        Normal = texture(uNormalTexture, vUV).rgb;
        Normal = normalize(Normal * 2.0 - 1.0);
        
        if(uUseTangentSpace) Normal = normalize(vTBN * Normal);

        if(showNormal)
        {   
            oColor = vec4(Normal,1.0);
            return;
        }
    }
    else
    {
        Normal = vNormal;
        if(showNormal)
        {   
            oColor = vec4(normalize(Normal),1.0);
            return;
        }
    }

    // Compute phong shading
    light_shade_result lightResult = get_lights_shading();
    
    vec3 diffuseColor  = gDefaultMaterial.diffuse * lightResult.diffuse * texture(uDiffuseTexture, vUV).rgb;
    vec3 ambientColor  = gDefaultMaterial.ambient * lightResult.ambient* texture(uDiffuseTexture, vUV).rgb;
    vec3 specularColor = gDefaultMaterial.specular * lightResult.specular;
    vec3 emissiveColor = gDefaultMaterial.emission + texture(uEmissiveTexture, vUV).rgb;
    
    // Apply light color
    oColor = vec4((ambientColor + diffuseColor + specularColor + emissiveColor), 1.0);
})GLSL";

static const char* gVDebugShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

// Uniforms
uniform mat4 uModel;
uniform mat4 uView;
uniform bool  uOrthogonize;

out VS_OUT {
    vec3 tangent;
    vec3 bitangent;
    vec3 normal;
}vs_out;

// Varyings

void main()
{
    mat4 MV = uView * uModel;
    gl_Position = MV * vec4(aPosition, 1.0);

    mat3 normalMatrix = mat3(transpose(inverse(MV)));

    //vs_out.normal =  normalize(normalMatrix * aNormal);
    //vs_out.tangent = normalize(normalMatrix * aTangent);

    vs_out.normal =  normalize(mat3(MV) * aNormal);
    vs_out.tangent = normalize(mat3(MV) * aTangent);

    if (uOrthogonize)
    {
        vs_out.tangent = normalize(vs_out.tangent - dot(vs_out.tangent , vs_out.normal) * vs_out.normal);
        vs_out.bitangent = cross(vs_out.normal, vs_out.tangent );
    }
    else
    {
        vs_out.bitangent = normalize(normalMatrix * aBitangent);
    }
})GLSL";


static const char* gGDebugShaderStr = R"GLSL(
layout(points) in;
layout(line_strip, max_vertices = 6) out;

uniform mat4 uProjection;

in VS_OUT {
    vec3 tangent;
    vec3 bitangent;
    vec3 normal;
}gs_in[];


out vec3 fColor;
const float length = 0.025;

const vec3 red = vec3(1.0,0.0,0.0);
const vec3 green = vec3(0.0,1.0,0.0);
const vec3 blue = vec3(0.0,0.0,1.0);

void GenerateLine(vec3 origin, vec3 dir, float len, vec3 color)
{
    fColor = color;

    gl_Position = uProjection * vec4(origin,1.0);
    EmitVertex();

    gl_Position = uProjection * vec4(origin + dir * len,1.0);
    EmitVertex();

    EndPrimitive();
}


void main()
{
    GenerateLine(gl_in[0].gl_Position.xyz, gs_in[0].tangent, length, red);
    GenerateLine(gl_in[0].gl_Position.xyz, gs_in[0].bitangent, length, green);
    GenerateLine(gl_in[0].gl_Position.xyz, gs_in[0].normal, length, blue);

})GLSL";

static const char* gFDebugShaderStr = R"GLSL(
in vec3 fColor;

out vec4 oColor;

void main()
{
    oColor = vec4(fColor, 1.0);
})GLSL";

demo_normal::demo_normal(GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug)//, scene_01(GLCache), scene_02(GLCache)
{
    scenes.push_back(new backpack_scene(GLCache));
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

            Program.push_back(GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, GLINCLUDE_PHONGLIGHT));


            glUseProgram(Program[i]);
            glUniform1i(glGetUniformLocation(Program[i], "uDiffuseTexture"), 0);
            glUniform1i(glGetUniformLocation(Program[i], "uEmissiveTexture"), 1);
            glUniform1i(glGetUniformLocation(Program[i], "uNormalTexture"), 2);
            glUniformBlockBinding(Program[i], glGetUniformBlockIndex(Program[i], "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
        }

        this->Debug = GL::CreateProgramEx(gVDebugShaderStr, gFDebugShaderStr, gGDebugShaderStr);
        //this->Debug = GL::CreateProgramEx(1,&gVDebugShaderStr,1, &gFDebugShaderStr);
    }
}

demo_normal::~demo_normal()
{
    // Cleanup GL
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
    glDeleteProgram(Debug);
}

void demo_normal::Update(const platform_io& IO)
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

    static double rotationTime = 0;
    if (Rotate)
    {
        rotationTime += IO.DeltaTime;
        ModelMatrix *= Mat4::RotateY((float)rotationTime * RotationSpeed);
    }

    // Render tavern
    this->Render(ProjectionMatrix, ViewMatrix, ModelMatrix);

    // Render tavern wireframe
    if (Wireframe)
    {
        GLDebug.Wireframe.BindBuffer(scenes[currentScene]->MeshBuffer, scenes[currentScene]->MeshDesc.Stride, scenes[currentScene]->MeshDesc.PositionOffset, scenes[currentScene]->MeshVertexCount);
        GLDebug.Wireframe.DrawArray(0, scenes[currentScene]->MeshVertexCount, ProjectionMatrix * ViewMatrix * ModelMatrix);
    }

    // Display debug UI
    this->DisplayDebugUI();
}

void demo_normal::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_normal", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        ImGui::SliderInt("Scene", &currentScene, 0,  (int)scenes.size()-1);

        ImGui::Checkbox("Rotate", &Rotate);
        if (Rotate)
        {
            ImGui::SliderFloat("    Speed", &RotationSpeed, -2.f, 2.f);
        }

        ImGui::Checkbox("Use Normal Map", &UseNormalMap);

        if (UseNormalMap)
        {
            ImGui::Checkbox("   Use Tangent Space", &UseTangentSpace);

            if (UseTangentSpace)
            {
                ImGui::Checkbox("       Show tangent space", &ShowTangentSpaces);
                ImGui::Checkbox("       Orthogonize tangent space", &Orthogonize);
            }

            ImGui::Checkbox("   Use Slider", &UseSlider);
            if (UseSlider)
            {
                ImGui::SliderFloat("        Slider Without / With Normal", &Slider, -1.f, 1.f);
                ImGui::Checkbox("       Show Half Normal", &ShowHalfNormal);
            }
        }

        ImGui::Checkbox("Show Normal Colors", &ShowNormals);

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

void demo_normal::Render(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
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
    glUniform1i(glGetUniformLocation(Program[currentScene], "uHasNormal"), UseNormalMap);
    glUniform1i(glGetUniformLocation(Program[currentScene], "uShowNormal"), ShowNormals);
    glUniform1i(glGetUniformLocation(Program[currentScene], "uShowHalfNormal"), ShowHalfNormal);
    glUniform1i(glGetUniformLocation(Program[currentScene], "uOrthogonize"), Orthogonize); 
    glUniform1i(glGetUniformLocation(Program[currentScene], "uUseSlider"), UseSlider);
    glUniform1f(glGetUniformLocation(Program[currentScene], "uSliderValue"), Slider); 
    glUniform1f(glGetUniformLocation(Program[currentScene], "uUseTangentSpace"), UseTangentSpace); 
    

    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, scenes[currentScene]->LightsUniformBuffer);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scenes[currentScene]->DiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, scenes[currentScene]->EmissiveTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, scenes[currentScene]->NormalTexture);
    glActiveTexture(GL_TEXTURE0); // Reset active texture just in case

    // Draw mesh
    scenes[currentScene]->DrawScene();

    if (ShowTangentSpaces == false) return;
    //  DEBUG
    glUseProgram(Debug);

    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(Debug, "uProjection"), 1, GL_FALSE, ProjectionMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Debug, "uModel"), 1, GL_FALSE, ModelMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(Debug, "uView"), 1, GL_FALSE, ViewMatrix.e);

    glUniform1i(glGetUniformLocation(Debug, "uOrthogonize"), Orthogonize);

    // Draw mesh
    scenes[currentScene]->DrawScene(GL_POINTS);
}
