#pragma once

#include <floral.h>
#include <math.h>

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

// up direction: positive Y

typedef floral::dynamic_array<VertexPN, FreelistArena>	TemporalVertices;
typedef floral::dynamic_array<floral::vec3f, FreelistArena>	TemporalQuads;
typedef floral::dynamic_array<u32, FreelistArena>		TemporalIndices;

// ---------------------------------------------
void											GenTessellated3DPlane_Tris(const floral::mat4x4f& i_xform,
													const f32 i_baseSize, const u32 i_gridsCount,
													TemporalVertices* o_vertices, TemporalIndices* o_indices);

void											GenTesselated3DPlane_Tris(const f32 i_width, const f32 i_height,
													const f32 i_gridSize,
													TemporalVertices* o_vertices, TemporalIndices* o_indices,
													const bool i_vtxDup = false);

void											GenQuadTesselated3DPlane_Tris(const f32 i_width, const f32 i_height,
													const f32 i_gridSize,
													TemporalVertices* o_vertices, TemporalIndices* o_indices,
													TemporalQuads* o_quadsList, const bool i_vtxDup = false);

void											GenTessellated3DPlane_TrisStrip(const floral::mat4x4f& i_xform,
													const f32 i_baseSize, const u32 i_gridsCount,
													TemporalVertices* o_vertices, TemporalIndices* o_indices);
// ---------------------------------------------
void											GenBox_Tris(const floral::mat4x4f& i_xform,
													TemporalVertices* o_vertices, TemporalIndices* o_indices);

void											GenBox_TrisStrip(const floral::mat4x4f& i_xform,
													TemporalVertices* o_vertices, TemporalIndices* o_indices);

// ---------------------------------------------
void											GenIcosphere_Tris(TemporalVertices* o_vertices, TemporalIndices* o_indices);

// ---------------------------------------------
template <typename TAllocator>
void											GenTesselated3DPlane_Tris_PNC(const floral::mat4x4f& i_xform,
													const f32 i_baseSize, const u32 i_gridsCount,
													const floral::vec4f& i_color,
													floral::fixed_array<VertexPNC, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

template <typename TAllocator>
void											GenTesselated3DPlane_Tris_PNC(const floral::mat4x4f& i_xform,
													const f32 i_width, const f32 i_height,
													const f32 i_gridSize, const floral::vec4f& i_color,
													floral::fixed_array<VertexPNC, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

template <typename TAllocator>
void											GenQuadTesselated3DPlane_Tris_PNC(const floral::mat4x4f& i_xform,
													const f32 i_width, const f32 i_height,
													const f32 i_gridSize, const floral::vec4f& i_color,
													floral::fixed_array<VertexPNC, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices,
													floral::fixed_array<GeoQuad, TAllocator>& o_quadsList);

template <typename TAllocator>
void											GenQuadTesselated3DPlane_Tris_PNCC(const floral::mat4x4f& i_xform,
													const f32 i_width, const f32 i_height,
													const f32 i_gridSize, const floral::vec4f& i_color,
													floral::fixed_array<VertexPNCC, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices,
													floral::fixed_array<GeoQuad, TAllocator>& o_quadsList);

template <typename TAllocator>
void											GenTessellated3DPlane_TrisStrip_PNC(const floral::mat4x4f& i_xform,
													const f32 i_baseSize, const u32 i_gridsCount,
													const floral::vec4f& i_color,
													floral::fixed_array<VertexPNC, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

template <typename TAllocator>
void											GenBox_Tris_PNC(const floral::mat4x4f& i_xform,
													const floral::vec4f& i_color,
													floral::fixed_array<VertexPNC, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

template <typename TAllocator>
void											GenBox_TrisStrip_PNC(const floral::mat4x4f& i_xform,
													const floral::vec4f& i_color,
													floral::fixed_array<VertexPNC, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

template <typename TAllocator>
void											GenIcosphere_Tris_P(const floral::mat4x4f& i_xform,
													floral::fixed_array<VertexP, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

template <typename TAllocator>
void											GenIcosphere_Tris_PNC(const floral::mat4x4f& i_xform,
													const floral::vec4f& i_color,
													floral::fixed_array<VertexP, TAllocator>& o_vertices,
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
void											GenBox_Tris_PosNormalColor(const floral::vec4f& i_color,
													const floral::mat4x4f& i_xform,
													floral::fixed_array<VertexPNC, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);

template <typename TAllocator>
void											GenIcosahedron_Tris_PosColor(const floral::vec4f& i_color,
													const floral::mat4x4f& i_xform,
													floral::fixed_array<DemoVertex, TAllocator>& o_vertices,
													floral::fixed_array<u32, TAllocator>& o_indices);


#endif
}

#include "GeometryBuilder.hpp"
