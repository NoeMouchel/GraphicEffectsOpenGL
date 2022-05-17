
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_shadowmap.h"

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
//uniform mat4 uMVPDepthMap;

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
uniform float uFarPlane;
uniform mat4 uMVPDepthMap[8];

uniform sampler2D uDiffuseTexture;
uniform sampler2D uEmissiveTexture;

//  DepthTextures
uniform sampler2D uDepthMap00;
uniform sampler2D uDepthMap01;
uniform sampler2D uDepthMap02;
uniform sampler2D uDepthMap03;
uniform sampler2D uDepthMap04;
uniform sampler2D uDepthMap05;
uniform sampler2D uDepthMap06;
uniform sampler2D uDepthMap07;

uniform samplerCube uDepthCubeMap00;
uniform samplerCube uDepthCubeMap01;
uniform samplerCube uDepthCubeMap02;
uniform samplerCube uDepthCubeMap03;
uniform samplerCube uDepthCubeMap04;
uniform samplerCube uDepthCubeMap05;
uniform samplerCube uDepthCubeMap06;
uniform samplerCube uDepthCubeMap07;

// Uniform blocks
layout(std140) uniform uLightBlock
{
	light uLight[LIGHT_COUNT];
};

// Shader outputs
out vec4 oColor;


sampler2D GetDepthMap(int i)
{
    if (i == 0) return uDepthMap00;
    else if (i == 1) return uDepthMap01;
    else if (i == 2)  return uDepthMap02;
    else if (i == 3) return uDepthMap03;
    else if (i == 4) return uDepthMap04;
    else if (i == 5) return uDepthMap05;
    else if (i == 6) return uDepthMap06;
    else if (i == 7) return uDepthMap07;
}

samplerCube GetDepthCubeMap(int i)
{
    if (i == 0) return uDepthCubeMap00;
    else if (i == 1) return uDepthCubeMap01;
    else if (i == 2) return uDepthCubeMap02;
    else if (i == 3) return uDepthCubeMap03;
    else if (i == 4) return uDepthCubeMap04;
    else if (i == 5) return uDepthCubeMap05;
    else if (i == 6) return uDepthCubeMap06;
    else if (i == 7) return uDepthCubeMap07;
}

//  Shadows Functions in opengl_helpers.cpp

float compute_visibility(int current)
{
    if(uLight[current].shadow == false || uLight[current].shadowGenerated == false) return 1.0;

    float visibility =1.0;

    if(uLight[current].type == 0 || uLight[current].type == 2)
    {
        visibility = 1.0 - shadow_compute_directionnal(GetDepthMap(current), uMVPDepthMap[current], vPos, vNormal, uLight[current].direction);
    }
    else
    {
        visibility = 1.0 - shadow_compute_point(GetDepthCubeMap(current), vPos,uViewPosition,  uLight[current].position, uFarPlane);
    }

    return visibility;
}

light_shade_result get_lights_shading()
{
    light_shade_result lightResult = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        
        light_shade_result light = light_shade(uLight[i], gDefaultMaterial.shininess, uViewPosition, vPos, normalize(vNormal));
        lightResult.ambient  += light.ambient;

        float visibility = compute_visibility(i);

        lightResult.diffuse  += light.diffuse * visibility;
        lightResult.specular += light.specular * visibility;
    }
    return lightResult;
}

void main()
{
    // Compute phong shading
    light_shade_result lightResult = get_lights_shading();
    
    vec3 diffuseColor  = gDefaultMaterial.diffuse * lightResult.diffuse * texture(uDiffuseTexture, vUV).rgb;
    vec3 ambientColor  = gDefaultMaterial.ambient * lightResult.ambient * texture(uDiffuseTexture, vUV).rgb;
    vec3 specularColor = gDefaultMaterial.specular * lightResult.specular;
    vec3 emissiveColor = gDefaultMaterial.emission + texture(uEmissiveTexture, vUV).rgb;

    // Apply light color
    oColor = vec4((ambientColor + diffuseColor + specularColor + emissiveColor), 1.0);
})GLSL";
#pragma endregion

#pragma region DepthMapVertexShader
static const char* gDepthMapVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;

// Uniforms
uniform mat4 uMVPDepthMap;

void main()
{
    gl_Position = uMVPDepthMap * vec4(aPosition, 1.0);
})GLSL";
#pragma endregion

#pragma region DepthMapFragmentShader
static const char* gDepthMapFragmentShaderStr = R"GLSL(
layout(location = 0) out float aFragDepth;

void main()
{
    //aFragDepth = gl_FragCoord.z;
})GLSL";
#pragma endregion

#pragma region DepthCubeMapVertexShader
static const char* gDepthCubeMapVertexShaderStr = R"GLSL(
// Attributes
layout(location = 0) in vec3 aPosition;

// Uniforms
uniform mat4 uModel;

void main()
{
    gl_Position = uModel * vec4(aPosition, 1.0);
})GLSL";
#pragma endregion

#pragma region DepthCubeMapGeometryShader
static const char* gDepthCubeMapGeometryShaderStr = R"GLSL(
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;


//  Uniforms
uniform mat4 uMVPDepthMap[6];

//  Shader Outputs
out vec4 oPosition;

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // built-in variable that specifies to which face we render.
        for(int i = 0; i < 3; ++i) // for each triangle vertex
        {
            oPosition = gl_in[i].gl_Position;
            gl_Position = uMVPDepthMap[face] * oPosition;
            EmitVertex();
        }    
        EndPrimitive();
    }
})GLSL";
#pragma endregion

#pragma region DepthCubeMapFragmentShader
static const char* gDepthCubeMapFragmentShaderStr = R"GLSL(

//  Varyings
in vec4 oPosition;

//  Uniforms
uniform vec3 uPosition;
uniform float uFarPlane;

void main()
{
    // get distance between fragment and light source
    float lightDistance = length(oPosition.xyz - uPosition);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / uFarPlane;
    
    // write this as modified depth
    gl_FragDepth = lightDistance;
})GLSL";
#pragma endregion


demo_shadowmap::demo_shadowmap(GL::cache& GLCache, GL::debug& GLDebug)
    : GLDebug(GLDebug), TavernScene(GLCache)
{
    // Create shader
    {
        // Assemble fragment shader strings (defines + code)
        char FragmentShaderConfig[] = "#define LIGHT_COUNT %d\n";
        snprintf(FragmentShaderConfig, ARRAY_SIZE(FragmentShaderConfig), "#define LIGHT_COUNT %d\n", TavernScene.LightCount);
        const char* FragmentShaderStrs[2] = {
            FragmentShaderConfig,
            gFragmentShaderStr,
        };

        this->Program = GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, GLINCLUDE_PHONGLIGHT | GLINCLUDE_SHADOW);
        this->DepthMapProgram = GL::CreateProgramEx(1, &gDepthMapVertexShaderStr, 1, &gDepthMapFragmentShaderStr);
        this->DepthCubeMapProgram = GL::CreateProgramEx(gDepthCubeMapVertexShaderStr, gDepthCubeMapFragmentShaderStr, gDepthCubeMapGeometryShaderStr);
    }

    // Create a vertex array and bind attribs onto the vertex buffer
    {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, TavernScene.MeshBuffer);

        vertex_descriptor& Desc = TavernScene.MeshDesc;
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.PositionOffset);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.UVOffset);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Desc.Stride, (void*)(size_t)Desc.NormalOffset);
    }


    {

        for (int i = 0; i < 8; i++)
        {
            //  DEPTH - MAP
            //  ---------------

            DepthMapFBO.push_back(0);
            glGenFramebuffers(1, &DepthMapFBO[i]);
            //glBindFramebuffer(GL_FRAMEBUFFER, DepthMapFBO[i]);


            DepthMap.push_back(0);

            glGenTextures(1, &DepthMap[i]);
            glBindTexture(GL_TEXTURE_2D, DepthMap[i]);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            glBindFramebuffer(GL_FRAMEBUFFER, DepthMapFBO[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, DepthMap[i], 0);

            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                std::printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

        }

        for (int i = 0; i < 8; i++)
        {
            //  DEPTH - CUBEMAP
            //  ---------------------

            DepthCubeMapFBO.push_back(0);
            glGenFramebuffers(1, &DepthCubeMapFBO[i]);

            DepthCubeMap.push_back(0);
            glGenTextures(1, &DepthCubeMap[i]);
            glBindTexture(GL_TEXTURE_CUBE_MAP, DepthCubeMap[i]);

            for (unsigned int i = 0; i < 6; ++i)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            glBindFramebuffer(GL_FRAMEBUFFER, DepthCubeMapFBO[i]);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, DepthCubeMap[i], 0);

            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                std::printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        for (int i = 0; i < 8; i++)
        {
            DepthMVP.push_back(std::vector<mat4>());
        }
    }

    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uDiffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(Program, "uEmissiveTexture"), 1);

        for (int i = 0; i < 8; i++)
        {
            std::string name = "uDepthMap0" + std::to_string(i);
            glUniform1i(glGetUniformLocation(Program, name.c_str()), 2 + i);
        }

        for (int i = 0; i < 8; i++)
        {
            std::string name = "uDepthCubeMap0" + std::to_string(i);
            glUniform1i(glGetUniformLocation(Program, name.c_str()), 2 + 8 + i);
        }

        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);
    }
    DepthMapsGeneration();
}

demo_shadowmap::~demo_shadowmap()
{
    // Cleanup GL
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(Program);
    glDeleteProgram(DepthMapProgram);
    glDeleteProgram(DepthCubeMapProgram);
}

void demo_shadowmap::Update(const platform_io& IO)
{
    const float AspectRatio = (float)IO.WindowWidth / (float)IO.WindowHeight;

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    // Clear screen
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DepthMapsGeneration();

    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    {
        mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
        mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
        mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });

        // Render tavern
        this->RenderTavern(ProjectionMatrix, ViewMatrix, ModelMatrix, DepthMVP);

        // Render tavern wireframe
        if (Wireframe)
        {
            GLDebug.Wireframe.BindBuffer(TavernScene.MeshBuffer, TavernScene.MeshDesc.Stride, TavernScene.MeshDesc.PositionOffset, TavernScene.MeshVertexCount);
            GLDebug.Wireframe.DrawArray(0, TavernScene.MeshVertexCount, ProjectionMatrix * ViewMatrix * ModelMatrix);
        }
    }

    // Display debug UI
    this->DisplayDebugUI();
}

void demo_shadowmap::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_shadowmap", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        ImGui::Checkbox("Wireframe", &Wireframe);

        static int i = 0;
        ImGui::SliderInt("Light DepthMap", &i, 0, TavernScene.LightCount - 1);

        if (TavernScene.GetLight(i)->Type == LIGHT_POINT)
        {
            ImGui::Text("CUBEMAP TEXTURE WON'T BE DSIPLAYED");
        }
        else
        {
            ImGui::Image((void*)(intptr_t)DepthMap[i], ImVec2(128, 128));
        }

        if (ImGui::TreeNodeEx("Camera"))
        {
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", Camera.Position.x, Camera.Position.y, Camera.Position.z);
            ImGui::Text("Pitch: %.2f", Math::ToDegrees(Camera.Pitch));
            ImGui::Text("Yaw: %.2f", Math::ToDegrees(Camera.Yaw));
            ImGui::TreePop();
        }
        TavernScene.InspectLights();

        ImGui::TreePop();
    }
}



void demo_shadowmap::DepthMapsGeneration()
{
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 1024, 1024);

    for (int i = 0; i < TavernScene.LightCount; i++)
    {
        GL::light* currentLight = TavernScene.GetLight(i);
        if (currentLight->Shadow == CASTSHADOW_NONE) continue;
        if (currentLight->Shadow == CASTSHADOW_STATIC && currentLight->ShadowGenerated) continue;


        if (currentLight->Type == LIGHT_POINT)
        {
            DepthMVP[i] = GetPointLightMVP(currentLight);
            GenerateDepthCubeMap(DepthMVP[i], i);
        }
        else
        {
            DepthMVP[i].push_back(mat4());
            DepthMVP[i][0] = GetLightMVP(currentLight);
            GenerateDepthMap(DepthMVP[i][0], i);
        }

        if (currentLight->ShadowGenerated == false)
        {
            currentLight->ShadowGenerated = true;
            glBindBuffer(GL_UNIFORM_BUFFER, TavernScene.LightsUniformBuffer);
            glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(GL::light), sizeof(GL::light), currentLight);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }
    }
    glDisable(GL_DEPTH_TEST);
}

void demo_shadowmap::GenerateDepthMap(const mat4& DepthMVP, const int current)
{
    glUseProgram(DepthMapProgram);

    glCullFace(GL_FRONT);

    glBindFramebuffer(GL_FRAMEBUFFER, DepthMapFBO[current]);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUniformMatrix4fv(glGetUniformLocation(DepthMapProgram, "uMVPDepthMap"), 1, GL_FALSE, DepthMVP.e);

    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, TavernScene.MeshVertexCount);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glCullFace(GL_BACK);
}

void demo_shadowmap::GenerateDepthCubeMap(const std::vector<mat4>& DepthMVP, const int current)
{
    glUseProgram(DepthCubeMapProgram);

    glCullFace(GL_FRONT);

    glBindFramebuffer(GL_FRAMEBUFFER, DepthCubeMapFBO[current]);
    glClear(GL_DEPTH_BUFFER_BIT);


    glUniformMatrix4fv(glGetUniformLocation(DepthCubeMapProgram, "uModel"), 1, GL_FALSE, Mat4::Identity().e);
    glUniform1f(glGetUniformLocation(DepthCubeMapProgram, "uFarPlane"), 25.0f);
    glUniform3fv(glGetUniformLocation(DepthCubeMapProgram, "uPosition"), 1, TavernScene.GetLight(current)->Position.e);

    for (int i = 0; i < 6; i++)
    {
        std::string name = "uMVPDepthMap[" + std::to_string(i) + "]";
        glUniformMatrix4fv(glGetUniformLocation(DepthCubeMapProgram, name.c_str()), 1, GL_FALSE, DepthMVP[i].e);
    }


    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, TavernScene.MeshVertexCount);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glCullFace(GL_BACK);
}



void demo_shadowmap::RenderTavern(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix, const std::vector< std::vector<mat4>>&  DepthMVP)
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

    // Bind uniform buffer and textures
    glBindBufferBase(GL_UNIFORM_BUFFER, LIGHT_BLOCK_BINDING_POINT, TavernScene.LightsUniformBuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TavernScene.DiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, TavernScene.EmissiveTexture);

    glActiveTexture(GL_TEXTURE0); // Reset active texture

    //      Lights

    for (int i = 0; i < TavernScene.LightCount; i++)
    {
        if (TavernScene.GetLight(i)->Shadow == CASTSHADOW_NONE) continue;

        if (TavernScene.GetLight(i)->Type == LIGHT_POINT)
        {
            glUniform1f(glGetUniformLocation(Program, "uFarPlane"), 25.0f);

            glActiveTexture(GL_TEXTURE2 + 8 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, DepthCubeMap[i]);
        }
        else
        {
            std::string name = "uMVPDepthMap[" + std::to_string(i) + "]";
            glUniformMatrix4fv(glGetUniformLocation(Program, name.c_str()), 1, GL_FALSE, DepthMVP[i][0].e);

            glActiveTexture(GL_TEXTURE2 + i);
            glBindTexture(GL_TEXTURE_2D, DepthMap[i]);
        }
    }

    glActiveTexture(GL_TEXTURE0);// Reset active texture

    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, TavernScene.MeshVertexCount);

    glDisable(GL_DEPTH_TEST);
}


mat4 demo_shadowmap::GetLightMVP(const GL::light* light)
{
    //  Directionnal Lights

    if (light->Type == LIGHT_DIRECTIONNAL)
    {
        v3 eye = { 0.0,2.0,0.0 };
        v3 at = eye + light->Direction;
        v3 up = { 0.f,1.f,0.f };
        mat4 depthProj = Mat4::Orthographic(-10, 10, -10, 10, -10, 20);
        mat4 depthView = Mat4::LookAt(eye, at, Vec3::Normalize(up));
        mat4 depthModel = Mat4::Identity();
        mat4 depthMVP = depthProj * depthView * depthModel;

        return depthMVP;
    }

    //  Spot lights

    if(light->Type == LIGHT_SPOT)
    {
         v3 target = light->Position + light->Direction;
        //v3 lightDir = light.Position.xyz;
    
         mat4 depthProj = Mat4::Perspective(Math::Acos(light->CutOff.y) * 2.f, 1.0f, 2.0f, 50.0f);
         mat4 depthView = Mat4::LookAt(light->Position, target, { 0.f,1.f,0.f });
         mat4 depthModel = Mat4::Identity();
         mat4 depthMVP = depthProj * depthView * depthModel;
    
         return depthMVP;
    }

    return Mat4::Identity();
}

struct CameraDirection
{
    GLenum CubemapFace;
    v3 Target;
    v3 Up;
};

CameraDirection gCameraDirections[6] =
{
    { GL_TEXTURE_CUBE_MAP_POSITIVE_X, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f} },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f} },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f} },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f} }
};

std::vector<mat4> demo_shadowmap::GetPointLightMVP(const GL::light* light)
{
    std::vector<mat4> out;

    for (int i = 0; i < 6; i++)
    {
        v3 target = light->Position + gCameraDirections[i].Target;

        mat4 depthProj = Mat4::Perspective(Math::ToRadians(90.0f), 1.0f, 1.0f, 25.0f);
        mat4 depthView = Mat4::LookAt(light->Position, target, gCameraDirections[i].Up);
        mat4 depthModel = Mat4::Identity();
        mat4 depthMVP = depthProj * depthView * depthModel;

        out.push_back(depthMVP);
    }

    return out;
}