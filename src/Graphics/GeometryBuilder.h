#pragma once

#include <floral.h>
#include <math.h>

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

// up direction: positive Y

typedef floral::dynamic_array<VertexPN, FreelistArena>	TemporalVertices;
typedef floral::dynamic_array<u32, FreelistArena>		TemporalIndices;

void											GenTessellated3DPlane_Tris(const floral::mat4x4f& i_xform,
													const f32 i_baseSize, const u32 i_gridsCount,
													TemporalVertices* o_vertices, TemporalIndices* o_indices);

// ---------------------------------------------
template <typename TAllocator>
void											GenTessellated3DPlane_Tris_PNC(const floral::mat4x4f& i_xform,
													const f32 i_baseSize, const u32 i_gridsCount,
													const floral::vec4f& i_color,
													floral::fixed_array<VertexPNC, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);
#if 0
template <typename TAllocator>
void											Gen3DPlane_Tris_PosColor(const floral::vec4f& i_color,
													const floral::mat4x4f& i_xform,
													floral::fixed_array<DemoVertex, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

template <typename TAllocator>
void											Gen3DPlane_Tris_PosNormalColor(const floral::vec4f& i_color,
													const floral::mat4x4f& i_xform,
													floral::fixed_array<VertexPNC, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

template <typename TAllocator>
void											GenBox_Tris_PosColor(const floral::vec4f& i_color,
													const floral::mat4x4f& i_xform,
													floral::fixed_array<DemoVertex, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

template <typename TAllocator>
void											GenBox_Tris_PosNormalColor(const floral::vec4f& i_color,
													const floral::mat4x4f& i_xform,
													floral::fixed_array<VertexPNC, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

template <typename TAllocator>
void											GenIcosahedron_Tris_PosColor(const floral::vec4f& i_color,
													const floral::mat4x4f& i_xform,
													floral::fixed_array<DemoVertex, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

template <typename TAllocator>
void											GenIcosphere_Tris_PosColor(const floral::vec4f& i_color,
													const floral::mat4x4f& i_xform,
													floral::fixed_array<DemoVertex, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

#endif
}

#include "GeometryBuilder.hpp"
