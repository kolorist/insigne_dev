#pragma once

#include <floral/stdaliases.h>
#include <floral/containers/array.h>
#include <floral/gpds/vec.h>

namespace stone
{

template <class TAllocator>
struct PlyData
{
	floral::fixed_array<floral::vec3f, TAllocator>	Position;
	floral::fixed_array<floral::vec3f, TAllocator>	Normal;
	floral::fixed_array<floral::vec2f, TAllocator>	TexCoord;
	floral::fixed_array<floral::vec3f, TAllocator>	Color;

	floral::fixed_array<s32, TAllocator>		Indices;

};

template <class TAllocator>
PlyData<TAllocator> LoadFromPly(const floral::path& i_path, TAllocator* i_allocator);

template <class TAllocator>
PlyData<TAllocator> LoadFFPatchesFromPly(const floral::path& i_path, TAllocator* i_allocator);

}

#include "PlyLoader.inl"
