
#include <cassert>
#include <vector>
#include <string>
#include <map>

#include <stb_image.h>

#include "platform.h"
#include "mesh.h"

#include "opengl_helpers.h"
#include "opengl_helpers_wireframe.h"

using namespace GL;

#pragma region ShaderStructs
static const char* ShaderStructsDefinitionsStr = R"GLSL(
#line 19
// Light structure
struct light
{
	int	type;
	bool enabled;
	bool shadow;
	bool shadowGenerated;

    vec3 position;
	vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 attenuation;

	vec2 cutOff;
};

struct light_shade_result
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 emission;
	float shininess;
};

struct Frag_PBR_material
{
	vec3 albedo;
	float metallic;
	float roughness;
	float ambientOcclusion;
};


// Default light
light gDefaultLight = light(
	1,
	true,
	true,
	false,

    vec3(1.0, 2.5, 0.0),
	vec3(0.0,-1.0,0.0),
    vec3(0.2, 0.2, 0.2),
    vec3(0.8, 0.8, 0.8),
    vec3(0.9, 0.9, 0.9),
    vec3(1.0, 0.0, 0.0),
    vec2(0.91, 0.82)
    );

// Default material
uniform material gDefaultMaterial = material(
    vec3(0.2, 0.2, 0.2),
    vec3(0.8, 0.8, 0.8),
    vec3(1.0, 1.0, 1.0),
    vec3(0.0, 0.0, 0.0),
    32.0);
)GLSL";
#pragma endregion

#pragma region ShaderPhong
// Light shader function
static const char* PhongLightingStr = R"GLSL(
#line 66
// =================================
// PHONG SHADER START ===============

// Phong shading function (model-space)
light_shade_result light_shade(light light, float shininess, vec3 eyePosition, vec3 position, vec3 normal)
{
	light_shade_result r = light_shade_result(vec3(0.0), vec3(0.0), vec3(0.0));
	if (!light.enabled)
		return r;

    vec3 lightDir;
    float lightAttenuation = 1.0;
	
	
	//	Point/Spot Light
    if (light.type > 0)
    {
        vec3 lightPosFromVertexPos = light.position - position;
        lightDir = normalize(lightPosFromVertexPos);
        float dist = length(lightPosFromVertexPos);
        lightAttenuation = 1.0 / (light.attenuation[0] + light.attenuation[1]*dist + light.attenuation[2]*light.attenuation[2]*dist);
    }

    // Directional light
    else if(light.type <= 0)
    {
        lightDir = -normalize(light.direction);
    }
	
    if (lightAttenuation < 0.001)
        return r;

    vec3 eyeDir  = normalize(eyePosition - position);
	vec3 reflectDir = reflect(-lightDir, normal);
	float specAngle = max(dot(reflectDir, eyeDir), 0.0);

    r.ambient  = lightAttenuation * light.ambient;
    r.diffuse  = lightAttenuation * light.diffuse  * max(dot(normal, lightDir), 0.0);
    r.specular = lightAttenuation * light.specular * (pow(specAngle, shininess / 4.0));
	r.specular = clamp(r.specular, 0.0, 1.0);

	//	Spot Light
	if(light.type == 2)
	{
		float theta = dot(lightDir, normalize(-light.direction)); 
		float epsilon = (light.cutOff.x - light.cutOff.y);
		float intensity = clamp((theta - light.cutOff.y) / epsilon, 0.0, 1.0);
		r.diffuse  *= intensity;
		r.specular *= intensity;
	}

	return r;
}
// PHONG SHADER STOP ===============
// =================================
)GLSL";
#pragma endregion

#pragma region ShaderShadows
// Shadows shader function
static const char* ShadowStr = R"GLSL(
#line 66
// =================================
// SHADOW SHADER START ===============

vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);


float shadow_compute_directionnal(sampler2D Text, mat4 MVPDepthMap, vec3 Pos, vec3 Normal, vec3 Direction)
{
    vec4 FragPosLightSpace = MVPDepthMap * vec4(Pos.xyz, 1.0);
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    //if(projCoords.z > 1.0) return 0.0;
    
    float closestDepth = texture(Text, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    float bias = max(0.005 * (1.0 - dot(Normal, normalize(-Direction))), 0.005);
    float shadow = 0.0;
   
    //  

    vec2 texelSize = 1.0 / textureSize(Text, 0);
   
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(Text, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }    
    return shadow / 9.0;
}


float shadow_compute_point(samplerCube Text, vec3 Pos, vec3 ViewPosition, vec3 LightPos, float FarPlane)
{
    vec3 lightToFrag = Pos - LightPos;
    float closestDepth = texture(Text, lightToFrag).r * FarPlane;
    float currentDepth = length(lightToFrag);

    float shadow = 0.0;
    float bias = 0.15;
    float samples = 20.0;
    float viewDistance = length(ViewPosition - Pos);
    float diskRadius = (1.0 + (viewDistance / FarPlane)) / 25.0;

    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(Text, lightToFrag + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= FarPlane;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }

    return shadow / float(samples);
}
// SHADOW SHADER STOP ===============
// =================================
)GLSL";
#pragma endregion

#pragma region ShaderKernel
// Kernel shader function
static const char* KernelStr = R"GLSL(
#line 66
// =================================
// KERNELS SHADER START ===============

const float offset = 1.0 / 300.0;  

float kernelBlur[9] = float[](
    1.0 / 16, 2.0 / 16, 1.0 / 16,
    2.0 / 16, 4.0 / 16, 2.0 / 16,
    1.0 / 16, 2.0 / 16, 1.0 / 16  
);

float kernelEdge[9] = float[](
    1.0, 1.0, 1.0,
    1.0, -8.0, 1.0,
    1.0, 1.0, 1.0  
);

vec2 offsets[9] = vec2[](
   vec2(-offset,  offset), // top-left
   vec2( 0.0f,    offset), // top-center
   vec2( offset,  offset), // top-right
   vec2(-offset,  0.0f),   // center-left
   vec2( 0.0f,    0.0f),   // center-center
   vec2( offset,  0.0f),   // center-right
   vec2(-offset, -offset), // bottom-left
   vec2( 0.0f,   -offset), // bottom-center
   vec2( offset, -offset)  // bottom-right    
);

vec4 ApplyKernel(float kernel[9], sampler2D text, vec2 UV)
{
    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture(text, UV.st + offsets[i]));
    }
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++)
        col += sampleTex[i] * kernel[i];

    return vec4(col,1.0);
}

// KERNELS SHADER STOP ===============
// =================================
)GLSL";
#pragma endregion

#pragma region ShaderPBR
// PBR shader function
static const char* PBRStr = R"GLSL(
#line 66
// =================================
// PBR SHADER START ===============

float PI = 3.141592653;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    //float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    //float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return NdotV / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

/*bool is_color_valid(vec3 color)
{
	return color.r >= 0.0;
}

bool is_value_valid(float value)
{
	return value >= 0.0;
}*/

vec3 compute_PBR_lighting(light currentLight, vec3 worldPos, vec3 normal, vec3 viewDir, Frag_PBR_material mat, vec3 reflection)
{
    float attenuation = 1.0;
	vec3 lightDir; 

	// calculate per-light radiance
	//	------------------------------
	
	// Point / Spot lights
    if (currentLight.type > 0)
    {
        vec3 lightPosFromVertexPos = currentLight.position - worldPos;
		lightDir = normalize(lightPosFromVertexPos);
        float dist = length(lightPosFromVertexPos);
        attenuation = 1.0 / (currentLight.attenuation[0] + currentLight.attenuation[1]*dist + currentLight.attenuation[2]*currentLight.attenuation[2]*dist);
	}
	//	Directionnal
	else
	{
        lightDir = -normalize(currentLight.direction);
	}

	vec3 H = normalize(viewDir + lightDir);
	vec3 radiance = currentLight.diffuse * attenuation;

	//	Spot Light
	if(currentLight.type == 2)
	{
		float theta = dot(lightDir, normalize(-currentLight.direction)); 
		float epsilon = (currentLight.cutOff.x - currentLight.cutOff.y);
		float intensity = clamp((theta - currentLight.cutOff.y) / epsilon, 0.0, 1.0);
		radiance  *= intensity;
	}

	float NDF = DistributionGGX(normal, H, mat.roughness);         
    float G   = GeometrySmith(normal, viewDir, lightDir, mat.roughness);
    vec3 F    = fresnelSchlick(max(dot(H, viewDir), 0.0), reflection); 
	
	vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - mat.metallic;	

	vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;  

	// add to outgoing radiance Lo
    float NdotL = max(dot(normal, lightDir), 0.0);

	return (kD * mat.albedo/ PI + specular) * radiance * NdotL; 
}


// PBR SHADER STOP ===============
// =================================
)GLSL";

#pragma endregion

void GL::UniformLight(GLuint Program, const char* LightUniformName, const light& Light)
{
	glUseProgram(Program);
	char UniformMemberName[255];

	sprintf(UniformMemberName, "%s.enabled", LightUniformName);
	glUniform1i(glGetUniformLocation(Program, UniformMemberName), Light.Enabled);

	sprintf(UniformMemberName, "%s.shadow", LightUniformName);
	glUniform1i(glGetUniformLocation(Program, UniformMemberName), Light.Shadow);

	sprintf(UniformMemberName, "%s.shadowGenerated", LightUniformName);
	glUniform1i(glGetUniformLocation(Program, UniformMemberName), Light.ShadowGenerated);

	sprintf(UniformMemberName, "%s.type", LightUniformName);
	glUniform1i(glGetUniformLocation(Program, UniformMemberName), Light.Type);

	sprintf(UniformMemberName, "%s.viewPosition", LightUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.Position.e);

	sprintf(UniformMemberName, "%s.direction", LightUniformName);
	glUniform4fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.Direction.e);

	sprintf(UniformMemberName, "%s.ambient", LightUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.Ambient.e);

	sprintf(UniformMemberName, "%s.diffuse", LightUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.Diffuse.e);

	sprintf(UniformMemberName, "%s.specular", LightUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.Specular.e);

	sprintf(UniformMemberName, "%s.attenuation", LightUniformName);
	glUniform1fv(glGetUniformLocation(Program, UniformMemberName), 3, Light.Attenuation.e);

	sprintf(UniformMemberName, "%s.cutOff", LightUniformName);
	glUniform2fv(glGetUniformLocation(Program, UniformMemberName), 1, Light.CutOff.e);
}

void GL::UniformMaterial(GLuint Program, const char* MaterialUniformName, const material& Material)
{
	glUseProgram(Program);
	char UniformMemberName[255];

	sprintf(UniformMemberName, "%s.ambient", MaterialUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Material.Ambient.e);

	sprintf(UniformMemberName, "%s.diffuse", MaterialUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Material.Diffuse.e);

	sprintf(UniformMemberName, "%s.specular", MaterialUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Material.Specular.e);

	sprintf(UniformMemberName, "%s.emission", MaterialUniformName);
	glUniform3fv(glGetUniformLocation(Program, UniformMemberName), 1, Material.Emission.e);

	sprintf(UniformMemberName, "%s.shininess", MaterialUniformName);
	glUniform1f(glGetUniformLocation(Program, UniformMemberName), Material.Shininess);
}


void GL::InjectIncludes(std::vector<const char*>& Sources, const int Includes)
{
	if (Includes)
	{
		Sources.push_back(ShaderStructsDefinitionsStr);
	}
	if (Includes & GLINCLUDE_KERNELS)
	{
		Sources.push_back(KernelStr);
	}

	if (Includes & GLINCLUDE_PHONGLIGHT)
	{
		Sources.push_back(PhongLightingStr);
	}

	if (Includes & GLINCLUDE_SHADOW)
	{
		Sources.push_back(ShadowStr);
	}

	if (Includes & GLINCLUDE_PBR)
	{
		Sources.push_back(PBRStr);
	}
	
}

GLuint GL::CompileShaderEx(GLenum ShaderType, int ShaderStrsCount, const char** ShaderStrs, const int Includes)
{
	GLuint Shader = glCreateShader(ShaderType);

	std::vector<const char*> Sources;
	Sources.reserve(4);
	Sources.push_back("#version 330 core\n");

	InjectIncludes(Sources, Includes);

	for (int i = 0; i < ShaderStrsCount; ++i)
		Sources.push_back(ShaderStrs[i]);

	glShaderSource(Shader, (GLsizei)Sources.size(), &Sources[0], nullptr);
	glCompileShader(Shader);

	GLint CompileStatus;
	glGetShaderiv(Shader, GL_COMPILE_STATUS, &CompileStatus);
	if (CompileStatus == GL_FALSE)
	{
		char Infolog[1024];
		glGetShaderInfoLog(Shader, ARRAY_SIZE(Infolog), nullptr, Infolog);
		fprintf(stderr, "Shader error: %s\n", Infolog);
	}

	return Shader;
}

GLuint GL::CompileShader(GLenum ShaderType, const char* ShaderStr, const int Includes)
{
	return GL::CompileShaderEx(ShaderType, 1, &ShaderStr,  Includes );
}

GLuint GL::CreateProgramEx(int VSStringsCount, const char** VSStrings, int FSStringsCount, const char** FSStrings, const int Includes)
{
	GLuint Program = glCreateProgram();

	GLuint VertexShader = GL::CompileShaderEx(GL_VERTEX_SHADER, VSStringsCount, VSStrings);
	GLuint FragmentShader = GL::CompileShaderEx(GL_FRAGMENT_SHADER, FSStringsCount, FSStrings,  Includes );

	glAttachShader(Program, VertexShader);
	glAttachShader(Program, FragmentShader);

	glLinkProgram(Program);
	GLint LinkStatus;
	glGetProgramiv(Program, GL_LINK_STATUS, &LinkStatus);
	if (LinkStatus == GL_FALSE)
	{
		char Infolog[1024];
		glGetProgramInfoLog(Program, ARRAY_SIZE(Infolog), nullptr, Infolog);
		fprintf(stderr, "Program link error: %s\n", Infolog);
	}
	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);

	return Program;
}

GLuint GL::CreateProgramEx(int VSStringsCount, const char** VSStrings, int FSStringsCount, const char** FSStrings, int GSStringsCount, const char** GSStrings, const int Includes)
{
	GLuint Program = glCreateProgram();

	GLuint VertexShader = GL::CompileShaderEx(GL_VERTEX_SHADER, VSStringsCount, VSStrings);
	GLuint FragmentShader = GL::CompileShaderEx(GL_FRAGMENT_SHADER, FSStringsCount, FSStrings,  Includes );
	GLuint GeometryShader = GL::CompileShaderEx(GL_GEOMETRY_SHADER, GSStringsCount, GSStrings);

	glAttachShader(Program, VertexShader);
	glAttachShader(Program, FragmentShader);
	glAttachShader(Program, GeometryShader);

	glLinkProgram(Program);
	GLint LinkStatus;
	glGetProgramiv(Program, GL_LINK_STATUS, &LinkStatus);
	if (LinkStatus == GL_FALSE)
	{
		char Infolog[1024];
		glGetProgramInfoLog(Program, ARRAY_SIZE(Infolog), nullptr, Infolog);
		fprintf(stderr, "Program link error: %s\n", Infolog);
	}


	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);
	glDeleteShader(GeometryShader);

	return Program;
}


GLuint GL::CreateProgramEx(const char* VSStrings, const char* FSStrings, const char* GSStrings, const int Includes)
{
	return CreateProgramEx(1, &VSStrings, 1, &FSStrings, 1, &GSStrings,  Includes );
}

GLuint GL::CreateProgram(const char* VSString, const char* FSString, const int Includes)
{
	return GL::CreateProgramEx(1, &VSString, 1, &FSString,  Includes );
}

const char* GL::GetShaderStructsDefinitions()
{
	return ShaderStructsDefinitionsStr;
}

//	Global Tables of formats
//	---------------------------

GLint GLImageFormat[] =
{
	-1, // 0 Channels, unused
	GL_RED,
	GL_RG,
	GL_RGB,
	GL_RGBA
};

GLint GLImageSRGBFormat[] =
{
	-1, // 0 Channels, unused
	GL_RED,
	GL_RG,
	GL_SRGB,
	GL_SRGB_ALPHA
};

void GetChannelsInfo(const int ImageFlags, int& DesiredChannels, int& Channels)
{
	if (ImageFlags & IMG_FORCE_GREY)
	{
		DesiredChannels = STBI_grey;
		Channels = 1;
	}
	if (ImageFlags & IMG_FORCE_GREY_ALPHA)
	{
		DesiredChannels = STBI_grey_alpha;
		Channels = 2;
	}
	if (ImageFlags & IMG_FORCE_RGB)
	{
		DesiredChannels = STBI_rgb;
		Channels = 3;
	}
	if (ImageFlags & IMG_FORCE_RGBA)
	{
		DesiredChannels = STBI_rgb_alpha;
		Channels = 4;
	}
}

void GL::UploadTexture(const char* Filename, int ImageFlags, int* WidthOut, int* HeightOut)
{
    // Flip
    stbi_set_flip_vertically_on_load((ImageFlags & IMG_FLIP) ? 1 : 0);

    // Desired channels
    int DesiredChannels = 0;
	int Channels = 0;
	GetChannelsInfo(ImageFlags, DesiredChannels, Channels);

    // Loading
    int Width, Height;
    uint8_t* Image = stbi_load(Filename, &Width, &Height, (DesiredChannels == 0) ? &Channels : nullptr, DesiredChannels);
    if (Image == nullptr)
    {
        fprintf(stderr, "Image loading failed on '%s'\n", Filename);
        return;
    }


	GLint internalFormat = ImageFlags & IMG_SRGB_SPACE ? GLImageSRGBFormat[Channels] : GLImageFormat[Channels];

    // Uploading
	if (ImageFlags & IMG_CUBEMAP)
	{
		static int i = 0;

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,	0, internalFormat, Width, Height, 0, GLImageFormat[Channels], GL_UNSIGNED_BYTE, Image);

		i = i < 6 ? i + 1 : 0;
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, Width, Height, 0, GLImageFormat[Channels], GL_UNSIGNED_BYTE, Image);
	}

    stbi_image_free(Image);

    // Mipmaps
    if (ImageFlags & IMG_GEN_MIPMAPS)
        glGenerateMipmap(GL_TEXTURE_2D);

    if (WidthOut)
        *WidthOut = Width;

    if (HeightOut)
        *HeightOut = Height;

    stbi_set_flip_vertically_on_load(0); // Always reset to default value
}


void GL::UploadCheckerboardTexture(int Width, int Height, int SquareSize)
{
	std::vector<v4> Texels(Width * Height);

	for (int y = 0; y < Height; ++y)
	{
		for (int x = 0; x < Width; ++x)
		{
			int PixelIndex = x + y * Width;
			int TileX = x / SquareSize;
			int TileY = y / SquareSize;
			Texels[PixelIndex] = ((TileX + TileY) % 2) ? v4{ 0.1f, 0.1f, 0.1f, 1.f } : v4{ 0.3f, 0.3f, 0.3f, 1.f };
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Width, 0, GL_RGBA, GL_FLOAT, &Texels[0]);
}
