#pragma once

#include <floral/cmds/path.h>

namespace cbmodel
{
// ------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)
struct ModelHeader
{
	s32											indicesCount;
	s32											verticesCount;

	size										indicesOffset;
	size										positionOffset;
	size										normalOffset;
	size										tangentOffset;
	size										texcoordOffset;
};
#pragma pack(pop)

template <class TVertex>
struct Model
{
	s32											indicesCount;
	s32											verticesCount;

	s32*										indicesData;
	TVertex*									verticesData;
	const_cstr									materialName;
};

enum class VertexAttribute : u32
{
	Invalid										= 0,
	Position									= 1,
	Normal										= Position << 1,
	Tangent										= Normal << 1,
	TexCoord									= Tangent << 1
};

inline VertexAttribute operator|(const VertexAttribute a, const VertexAttribute b)
{
	return (VertexAttribute)(u32(a) | u32(b));
}

template <class TVertex, class TIOAllocator, class TDataAllocator>
const Model<TVertex>							LoadModelData(const floral::path& i_path, const VertexAttribute i_vtxAttrib, TIOAllocator* i_ioAllocator, TDataAllocator* i_dataAllocator);

// ------------------------------------------------------------------
}

#include "CbModelLoader.inl"
