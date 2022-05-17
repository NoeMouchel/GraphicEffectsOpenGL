
#include <vector>

#include <imgui.h>

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

#include "color.h"
#include "maths.h"
#include "mesh.h"

#include "demo_postprocess.h"

const int LIGHT_BLOCK_BINDING_POINT = 0;

#pragma region DefaultVertextShader
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

uniform sampler2D uDiffuseTexture;
uniform sampler2D uEmissiveTexture;

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

void main()
{
    // Compute phong shading
    light_shade_result lightResult = get_lights_shading();
    
    vec3 diffuseColor  = gDefaultMaterial.diffuse * lightResult.diffuse * texture(uDiffuseTexture, vUV).rgb;
    vec3 ambientColor  = gDefaultMaterial.ambient * lightResult.ambient* texture(uDiffuseTexture, vUV).rgb;
    vec3 specularColor = gDefaultMaterial.specular * lightResult.specular;
    vec3 emissiveColor = gDefaultMaterial.emission + texture(uEmissiveTexture, vUV).rgb;
    
    // Apply light color
    oColor = vec4((ambientColor + diffuseColor + specularColor + emissiveColor), 1.0);
})GLSL";
#pragma endregion

#pragma region PostProcessVertextShader
static const char* gPostprocessVertexShaderStr = R"GLSL(
//  Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;

//  Varyings
out vec2 vUV;

//  Uniforms
uniform bool uShake;
uniform float uShakeTime;

void main()
{
    vUV = aUV;
    gl_Position = vec4(aPosition.x, aPosition.y, 0.0, 1.0);

    if (uShake)
    {
        float strength = 0.005;
        gl_Position.x += cos(uShakeTime * 20) * strength;
        gl_Position.y += cos(uShakeTime * 25) * strength;
    }
})GLSL";
#pragma endregion

#pragma region PostProcessFragmentShader
static const char* gPostprocessFragmentShaderStr = R"GLSL(
//  Varyings
in vec2 vUV;

//  Uniforms
uniform int uMode;
uniform bool uShake;
uniform sampler2D uScreenTexture;

//  Shader outputs
out vec4 oColor;

void main()
{    
    if(uMode == 0)
    {
        oColor = texture(uScreenTexture, vUV);
    }
    else if (uMode == 1)
    {
        oColor = vec4(vec3(1.0 - texture(uScreenTexture, vUV)), 1.0);
    }
    else if (uMode == 2)
    {
        oColor = texture(uScreenTexture, vUV);
        float average = (oColor.r + oColor.g + oColor.b) / 3.0;
        oColor = vec4(average, average, average, 1.0);
    }
    else if (uMode == 3)
    {    
        oColor = ApplyKernel(kernelBlur,uScreenTexture,vUV);
    }
    else if (uMode == 4)
    {    
        oColor = ApplyKernel(kernelEdge,uScreenTexture,vUV);
    }

    if(uShake)
    {
        oColor = ApplyKernel(kernelBlur,uScreenTexture,vUV);
    }
})GLSL";
#pragma endregion


float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
};


enum modes
{
    MODE_NORMAL = 0,
    MODE_INVERSE = 1,
    MODE_GRAYSCALE = 2,
    MODE_KERNEL_BLUR = 3,
    MODE_KERNEL_EDGE = 4,
    COUNT = 6
};


demo_postprocess::demo_postprocess(GL::cache& GLCache, GL::debug& GLDebug)
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

        this->Program = GL::CreateProgramEx(1, &gVertexShaderStr, 2, FragmentShaderStrs, GLINCLUDE_PHONGLIGHT);

        this->PostProcessProgram = GL::CreateProgramEx(1, &gPostprocessVertexShaderStr, 1, &gPostprocessFragmentShaderStr, GLINCLUDE_KERNELS);
    }
    
    //  Create post-process quad
    {
        GLuint quadVBO;
        glGenVertexArrays(1, &ScreenVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(ScreenVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
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

    // Set uniforms that won't change
    {
        glUseProgram(Program);
        glUniform1i(glGetUniformLocation(Program, "uDiffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(Program, "uEmissiveTexture"), 1);
        glUniformBlockBinding(Program, glGetUniformBlockIndex(Program, "uLightBlock"), LIGHT_BLOCK_BINDING_POINT);

        glUseProgram(PostProcessProgram);
        glUniform1i(glGetUniformLocation(Program, "uScreenTexture"), 0);
    }

    //  Generate postprocess frame buffer
    {
        glGenFramebuffers(1, &ScreenFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, ScreenFBO);

        //  Generate screen texture
        glGenTextures(1, &ScreenTexture);
        glBindTexture(GL_TEXTURE_2D, ScreenTexture);

        ScreenWidth = 1440;
        ScreenHeight = 900;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ScreenWidth, ScreenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ScreenTexture, 0);

        //  Generate RBO
        glGenRenderbuffers(1, &ScreenRBO);

        glBindRenderbuffer(GL_RENDERBUFFER, ScreenRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, ScreenWidth, ScreenHeight);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, ScreenRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

demo_postprocess::~demo_postprocess()
{
    // Cleanup GL
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(Program);
}

void demo_postprocess::Update(const platform_io& IO)
{
    int width = IO.WindowWidth;
    int height = IO.WindowHeight;
    const float AspectRatio = (float)width / (float)height;

    if (ScreenWidth != width || ScreenHeight != height)
    {
        //  Update size of texture and render buffer
        ScreenWidth = width;
        ScreenHeight = height;

        glBindTexture(GL_TEXTURE_2D, ScreenTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glBindRenderbuffer(GL_RENDERBUFFER, ScreenRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    }

    glViewport(0, 0, IO.WindowWidth, IO.WindowHeight);

    Camera = CameraUpdateFreefly(Camera, IO.CameraInputs);

    // Clear screen
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 ProjectionMatrix = Mat4::Perspective(Math::ToRadians(60.f), AspectRatio, 0.1f, 100.f);
    mat4 ViewMatrix = CameraGetInverseMatrix(Camera);
    mat4 ModelMatrix = Mat4::Translate({ 0.f, 0.f, 0.f });

    // Render tavern
    PreProcess();
    this->RenderScene(ProjectionMatrix, ViewMatrix, ModelMatrix);
    PostProcess();

    // Render tavern wireframe
    if (Wireframe)
    {
        GLDebug.Wireframe.BindBuffer(TavernScene.MeshBuffer, TavernScene.MeshDesc.Stride, TavernScene.MeshDesc.PositionOffset, TavernScene.MeshVertexCount);
        GLDebug.Wireframe.DrawArray(0, TavernScene.MeshVertexCount, ProjectionMatrix * ViewMatrix * ModelMatrix);
    }

    //  Shake timing
    if (Shake)
    {
        ElapsedTimeShaking += (float)IO.DeltaTime;
        if (ShakeTime < ElapsedTimeShaking)
        {
            ElapsedTimeShaking = 0.f;
            Shake = false;
        }
    }

    // Display debug UI
    this->DisplayDebugUI();
}

void demo_postprocess::DisplayDebugUI()
{
    if (ImGui::TreeNodeEx("demo_postprocess", ImGuiTreeNodeFlags_Framed))
    {
        // Debug display
        ImGui::Checkbox("Wireframe", &Wireframe);

        static int selected_radio;
        if (ImGui::RadioButton("Normal Mode", &selected_radio, 0))
        {
            Mode = (int)MODE_NORMAL;
        }
        if (ImGui::RadioButton("Inverse Mode", &selected_radio, 1))
        {
            Mode = (int)MODE_INVERSE;
        }
        if (ImGui::RadioButton("Grayscale Mode", &selected_radio, 2))
        {
            Mode = (int)MODE_GRAYSCALE;
        }
        if (ImGui::RadioButton("Kernel Edge Mode", &selected_radio, 3))
        {
            Mode = (int)MODE_KERNEL_EDGE;
        }
        if (ImGui::RadioButton("Kernel Blur Mode", &selected_radio, 4))
        {
            Mode = (int)MODE_KERNEL_BLUR;
        }


        if (ImGui::Button("Shake"))
        {
            Shake = true;
            
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


void demo_postprocess::PreProcess()
{
    glBindFramebuffer(GL_FRAMEBUFFER, ScreenFBO);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // we're not using the stencil buffer now
    glEnable(GL_DEPTH_TEST);
}


void demo_postprocess::PostProcess()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(PostProcessProgram);

    int inShake = (int)Shake;
    glUniform1iv(glGetUniformLocation(PostProcessProgram, "uShake"), 1, &inShake);
    if(Shake) glUniform1fv(glGetUniformLocation(PostProcessProgram, "uShakeTime"), 1, &ElapsedTimeShaking);

    glUniform1iv(glGetUniformLocation(PostProcessProgram, "uMode"), 1, &Mode);

    glBindVertexArray(ScreenVAO);
    glBindTexture(GL_TEXTURE_2D, ScreenTexture);	// use the color attachment texture as the texture of the quad plane
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void demo_postprocess::RenderScene(const mat4& ProjectionMatrix, const mat4& ViewMatrix, const mat4& ModelMatrix)
{
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
    glActiveTexture(GL_TEXTURE0); // Reset active texture just in case
    
    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, TavernScene.MeshVertexCount);
}
