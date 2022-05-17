
#include <imgui.h>

#include "platform.h"

#include "color.h"

#include "scene.h"


scene::~scene()
{
    glDeleteBuffers(1, &LightsUniformBuffer);
    glDeleteVertexArrays(1, &VAO);
}

void scene::CreateMesh(GL::cache& GLCache, const char* filepath)
{
    // Use vbo from GLCache
    MeshBuffer = GLCache.LoadObj(filepath, 1.f, &this->MeshVertexCount);

    MeshDesc.Stride = sizeof(vertex_full);
    MeshDesc.HasNormal = true;
    MeshDesc.HasUV = true;
    MeshDesc.PositionOffset = OFFSETOF(vertex_full, Position);
    MeshDesc.NormalOffset = OFFSETOF(vertex_full, Normal);
    MeshDesc.UVOffset = OFFSETOF(vertex_full, UV);

    MeshDesc.TangentOffset = OFFSETOF(vertex_full, Tangents);
    MeshDesc.BitangentOffset = OFFSETOF(vertex_full, Bitangents);
}

static bool EditLight(GL::light* Light)
{
    bool Result = ImGui::Checkbox("Enabled", (bool*)&Light->Enabled);

    if (!Light->Enabled) return Result;

    Result |= ImGui::SliderInt("Type", &Light->Type, 0, 2)
        | ImGui::SliderFloat3("Position", Light->Position.e, -10.f, 10.f);

    if (Light->Type == LIGHT_SPOT || Light->Type == LIGHT_DIRECTIONNAL)
    {
        Result |= ImGui::SliderFloat3("Direction", Light->Direction.e, -1.f, 1.f);
    }

    Result |= ImGui::ColorEdit3("Ambient", Light->Ambient.e)
        | ImGui::ColorEdit3("Diffuse", Light->Diffuse.e)
        | ImGui::ColorEdit3("Specular", Light->Specular.e)
        | ImGui::SliderFloat("Attenuation (constant)", &Light->Attenuation.e[0], 0.f, 10.f)
        | ImGui::SliderFloat("Attenuation (linear)", &Light->Attenuation.e[1], 0.f, 10.f)
        | ImGui::SliderFloat("Attenuation (quadratic)", &Light->Attenuation.e[2], 0.f, 10.f);

    if (Light->Type == LIGHT_SPOT)
    {
        Result |= ImGui::SliderFloat("Inner CutOff", &Light->CutOff.x, 0.f, 1.f)
            | ImGui::SliderFloat("Outer CutOff", &Light->CutOff.y, 0.f, 1.f);
    }

    int selected_radio = Light->Shadow;
    int oldSelectedRadio = selected_radio;
    if (ImGui::RadioButton("No Shadow", &selected_radio, 0))
    {
        Light->Shadow = (int)CASTSHADOW_NONE;
    }
    if (ImGui::RadioButton("Static Shadows", &selected_radio, 1))
    {
        Light->Shadow = (int)CASTSHADOW_STATIC;
    }
    if (ImGui::RadioButton("Dynamic Shadows", &selected_radio, 2))
    {
        Light->Shadow = (int)CASTSHADOW_DYNAMIC;
    }
    Result |= (oldSelectedRadio != selected_radio);

    return Result;
}

void scene::InspectLights()
{
    if (ImGui::TreeNodeEx("Lights"))
    {
        for (int i = 0; i < LightCount; ++i)
        {
            if (ImGui::TreeNode(&Lights[i], "Light[%d]", i))
            {
                GL::light& Light = Lights[i];
                if (EditLight(&Light))
                {
                    Light.ShadowGenerated = false;
                    glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(GL::light), sizeof(GL::light), &Light);
                }

                // Calculate attenuation based on the light values
                if (ImGui::TreeNode("Attenuation calculator"))
                {
                    static float Dist = 5.f;
                    float Att = 1.f / (Light.Attenuation.e[0] + Light.Attenuation.e[1] * Dist + Light.Attenuation.e[2] * Light.Attenuation.e[2] * Dist);
                    ImGui::Text("att(d) = 1.0 / (c + ld + qdd)");
                    ImGui::SliderFloat("d", &Dist, 0.f, 20.f);
                    ImGui::Text("att(%.2f) = %.2f", Dist, Att);
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
}

void scene::GenerateVAO()
{
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, MeshBuffer);

    {
        vertex_descriptor& Desc_01 = MeshDesc;
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Desc_01.Stride, (void*)(size_t)Desc_01.PositionOffset);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Desc_01.Stride, (void*)(size_t)Desc_01.UVOffset);

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, Desc_01.Stride, (void*)(size_t)Desc_01.NormalOffset);

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, Desc_01.Stride, (void*)(size_t)Desc_01.TangentOffset);

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, Desc_01.Stride, (void*)(size_t)Desc_01.BitangentOffset);
    }
}

void scene::DrawScene(GLenum mode)
{
    glBindVertexArray(VAO);
    glDrawArrays(mode, 0, MeshVertexCount);
}