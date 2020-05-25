#include "Triangle.h"

#include <floral/containers/array.h>
#include <floral/io/filesystem.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/MaterialParser.h"

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------

Triangle::Triangle()
{
}

//-------------------------------------------------------------------

Triangle::~Triangle()
{
}

//-------------------------------------------------------------------

ICameraMotion* Triangle::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr Triangle::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void Triangle::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	floral::relative_path wdir = floral::build_relative_path("tests/perf/triangle");
	floral::push_directory(m_FileSystem, wdir);

	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(2));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(1));

	// register surfaces
	insigne::register_surface_type<geo2d::SurfacePC>();

	floral::inplace_array<geo2d::VertexPC, 3> vertices;
	vertices.push_back(geo2d::VertexPC { { 0.0f, 0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f } });
	vertices.push_back(geo2d::VertexPC { { -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f } });
	vertices.push_back(geo2d::VertexPC { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f } });

	floral::inplace_array<s32, 3> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(2);
		desc.stride = sizeof(geo2d::VertexPC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_VB = insigne::create_vb(desc);
		insigne::copy_update_vb(m_VB, &vertices[0], vertices.get_size(), sizeof(geo2d::VertexPC), 0);
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(1);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_IB = insigne::create_ib(desc);
		insigne::copy_update_ib(m_IB, &indices[0], indices.get_size(), 0);
	}

	floral::relative_path matPath = floral::build_relative_path("triangle.mat");
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	bool matLoadResult = mat_loader::CreateMaterial(&m_MSPair, m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(matLoadResult);
}

//-------------------------------------------------------------------

void Triangle::_OnUpdate(const f32 i_deltaMs)
{
}

//-------------------------------------------------------------------

void Triangle::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<geo2d::SurfacePC>(m_VB, m_IB, m_MSPair.material);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

//-------------------------------------------------------------------

void Triangle::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	insigne::unregister_surface_type<geo2d::SurfacePC>();

	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_MemoryArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------
}
}
