#pragma once

#include <vector>

#include "types.h"

// Descriptor for interleaved vertex formats
struct vertex_descriptor
{
	int Stride;

	int PositionOffset;

	bool HasNormal;
	int NormalOffset;

	bool HasUV;
	int UVOffset;

	int TangentOffset;
	int BitangentOffset;
};

struct vertex_full
{
	v3 Position;
	v3 Normal = { 0.f,1.f,0.f };
	v2 UV;

	v3 Tangents = { 1.f , 0.f , 0.f };
	v3 Bitangents = { 0.f , 0.f , 1.f };
};

namespace Mesh
{
	void* Transform(void* Vertices, void* End, const vertex_descriptor& Descriptor, const mat4& Transform);

	void* BuildQuad(void* Vertices, void* End, const vertex_descriptor& Descriptor);

	void* BuildCube(void* Vertices, void* End, const vertex_descriptor& Descriptor);

	void* BuildInvertedCube(void* Vertices, void* End, const vertex_descriptor& Descriptor);

	void* BuildSphere(void* Vertices, void* End, const vertex_descriptor& Descriptor, int Lon, int Lat);

	void* LoadObj(void* Vertices, void* End, const vertex_descriptor& Descriptor, const char* Filename, float Scale);

	bool LoadObjNoConvertion(std::vector<vertex_full>& Mesh, const char* Filename, float Scale);

	void ComputeTangentBasis(std::vector<vertex_full>& mesh);
}