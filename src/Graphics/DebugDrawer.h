#pragma once

#include <floral.h>

#include <insigne/commons.h>

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone
{

class DebugDrawer
{
typedef floral::fixed_array<VertexPC, LinearArena> VerticesArray;
typedef floral::fixed_array<u32, LinearArena> IndicesArray;

public:
	DebugDrawer();
	~DebugDrawer();

	// -------------------------------------
	template <class TAllocator>
	void										DrawPolygon3D(const floral::fixed_array<floral::vec3f, TAllocator>& i_polySoup, const floral::vec4f& i_color);
	void										DrawLine3D(const floral::vec3f& i_x0, const floral::vec3f& i_x1, const floral::vec4f& i_color);
	void										DrawAABB3D(const floral::aabb3f& i_aabb, const floral::vec4f& i_color);
	void										DrawQuad3D(const floral::vec3f& i_p0, const floral::vec3f& i_p1, const floral::vec3f& i_p2, const floral::vec3f& i_p3, const floral::vec4f& i_color);
	//void										DrawOBB3D(const floral::obb3f& i_obb);
	void										DrawIcosahedron3D(const floral::vec3f& i_origin, const f32 i_radius, const floral::vec4f& i_color);
	void										DrawIcosphere3D(const floral::vec3f& i_origin, const f32 i_radius, const floral::vec4f& i_color);
	// -------------------------------------
	void										Initialize();
	void										CleanUp();

	void										Render(const floral::mat4x4f& i_wvp);

	void										BeginFrame();
	void										EndFrame();

private:
	s32										m_CurrentBufferIdx;
	VerticesArray							m_DebugVertices[2];
	IndicesArray							m_DebugIndices[2];

	struct MyData {
		floral::mat4x4f						WVP;
	};
	MyData									m_Data[2];

	insigne::vb_handle_t					m_VB;
	insigne::ib_handle_t					m_IB;
	insigne::ub_handle_t					m_UB;
	insigne::shader_handle_t				m_Shader;
	insigne::material_desc_t				m_Material;

	LinearArena*							m_MemoryArena;

	// resource control
private:
	ssize										m_BuffersBeginStateId;
	ssize										m_ShadingBeginStateId;
	ssize										m_TextureBeginStateId;
	ssize										m_RenderBeginStateId;
};

//----------------------------------------------
namespace debugdraw
{
void											Initialize();
void											CleanUp();

void											BeginFrame();
void											EndFrame();
void											Render(const floral::mat4x4f& i_wvp);

void											DrawLine3D(const floral::vec3f& i_x0, const floral::vec3f& i_x1, const floral::vec4f& i_color);
void											DrawQuad3D(const floral::vec3f& i_p0, const floral::vec3f& i_p1, const floral::vec3f& i_p2, const floral::vec3f& i_p3, const floral::vec4f& i_color);
}

}

#include "DebugDrawer.hpp"
