#pragma once

#include <floral.h>
#include <floral/io/filesystem.h>

#include <insigne/commons.h>

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/stb_truetype.h"

namespace stone
{

class DebugDrawer
{
typedef floral::fixed_array<VertexPC, LinearArena> VerticesArray;
typedef floral::fixed_array<u32, LinearArena> IndicesArray;

typedef floral::fixed_array<DebugTextVertex, LinearArena> TextVerticesArray;

struct GlyphInfo
{
    s32                                     codePoint;
    s32                                     width, height;
    s32                                     advanceX, offsetX, offsetY;
    s32                                     aX, aY;

    p8                                      bmData;
};
typedef floral::fixed_array<GlyphInfo, LinearArena> GlyphsArray;

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

	void										DrawPoint3D(const floral::vec3f& i_position, const f32 i_size, const floral::vec4f& i_color);
	void										DrawSolidBox3D(const floral::vec3f& i_minCorner, const floral::vec3f& i_maxCorner, const floral::vec4f& i_color);

	void										DrawText3D(const_cstr i_str, const floral::vec3f& i_position);
	// -------------------------------------
	void										Initialize(floral::filesystem<FreelistArena>* i_fs);
	void										CleanUp();

	void										Render(const floral::mat4x4f& i_wvp);

	void										BeginFrame();
	void										EndFrame();

private:
	s32											m_CurrentBufferIdx;
	VerticesArray								m_DebugVertices[2];
	IndicesArray								m_DebugIndices[2];
	VerticesArray								m_DebugSurfaceVertices[2];
	IndicesArray								m_DebugSurfaceIndices[2];
	TextVerticesArray							m_DebugTextVertices[2];
	IndicesArray								m_DebugTextIndices[2];
    GlyphsArray                                 m_Glyphs;

	struct MyData {
		floral::mat4x4f							WVP;
	};
	MyData										m_Data[2];

	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;
	insigne::vb_handle_t						m_SurfaceVB;
	insigne::ib_handle_t						m_SurfaceIB;
	insigne::vb_handle_t						m_TextVB;
	insigne::ib_handle_t						m_TextIB;
	insigne::ub_handle_t						m_UB;
	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;
	insigne::texture_handle_t					m_FontAtlas;
	insigne::shader_handle_t					m_TextShader;
	insigne::material_desc_t					m_TextMaterial;

	LinearArena*								m_MemoryArena;

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
void											Initialize(floral::filesystem<FreelistArena>* i_fs);
void											CleanUp();

void											BeginFrame();
void											EndFrame();
void											Render(const floral::mat4x4f& i_wvp);

void											DrawLine3D(const floral::vec3f& i_x0, const floral::vec3f& i_x1, const floral::vec4f& i_color);
void											DrawQuad3D(const floral::vec3f& i_p0, const floral::vec3f& i_p1, const floral::vec3f& i_p2, const floral::vec3f& i_p3, const floral::vec4f& i_color);
void											DrawAABB3D(const floral::aabb3f& i_aabb, const floral::vec4f& i_color);

void											DrawPoint3D(const floral::vec3f& i_position, const f32 i_size, const floral::vec4f& i_color);
void											DrawPoint3D(const floral::vec3f& i_position, const f32 i_size, const size i_colorIdx);

void											DrawText3D(const_cstr i_str, const floral::vec3f& i_position);
}

}

#include "DebugDrawer.hpp"
