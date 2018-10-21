#pragma once

#include <math.h>

#include "Graphics/SurfaceDefinitions.h"

namespace stone {

// up direction: positive Y

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

}

#include "GeometryBuilder.hpp"
