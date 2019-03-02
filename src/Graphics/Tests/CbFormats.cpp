#include "CbFormats.h"

#include <clover.h>
#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include "Graphics/GeometryBuilder.h"
#include "Graphics/CBObjLoader.h"

namespace stone {

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

out mediump vec4 v_VertexColor;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_VertexColor = l_Color;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_VertexColor;

void main()
{
	o_Color = v_VertexColor;
}
)";

/* TODO
 * 1. Debug UI
 * 2. Camera movement
 * 3. Geometry algos: octree partitioning
 */

CbFormats::CbFormats()
	: m_CameraMotion(floral::vec3f(5.0f, 5.0f, 5.0f), floral::vec3f(0.0f, 1.0f, 0.0f), floral::vec3f(-5.0f, -5.0f, -5.0f))
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
	m_PlyReaderArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));

	m_CameraMotion.SetProjection(0.01f, 100.0f, 60.0f, 16.0f / 9.0f);
}

CbFormats::~CbFormats()
{
}

void CbFormats::OnInitialize()
{
	{	
		m_MemoryArena->free_all();
		floral::file_info scnFile = floral::open_file("gfx/go/models/bathroom/scene.cbscn");
		floral::file_stream dataStream;
		dataStream.buffer = (p8)m_MemoryArena->allocate(scnFile.file_size);
		floral::read_all_file(scnFile, dataStream);
		floral::close_file(scnFile);

		floral::mat4x4f worldXForm(1.0f);
		dataStream.read(&worldXForm);

		u32 meshCount = 0;
		dataStream.read(&meshCount);

		m_VBs.init(meshCount, &g_PersistanceResourceAllocator);
		m_IBs.init(meshCount, &g_PersistanceResourceAllocator);

		for (u32 i = 0; i < meshCount; i++)
		{
			u32 meshPathLen = 0;
			dataStream.read(&meshPathLen);
			c8 meshPath[1024];
			memset(meshPath, 0, 1024);
			dataStream.read_bytes(meshPath, meshPathLen);
			CLOVER_DEBUG("meshPath: %s", meshPath);

			m_PlyReaderArena->free_all();
			cb::ModelLoader<LinearArena> loader(m_PlyReaderArena);
			c8 pathStr[1024];
			memset(pathStr, 0, 1024);
			sprintf(pathStr, "%s/%s", "gfx/go/models/bathroom", meshPath);
			loader.LoadFromFile(floral::path(pathStr));

			floral::fixed_array<VertexPC, LinearArena> vertices(loader.GetVerticesCount(0), m_PlyReaderArena);
			floral::fixed_array<s32, LinearArena> indices(loader.GetIndicesCount(0), m_PlyReaderArena);
			vertices.resize_ex(loader.GetVerticesCount(0));
			indices.resize_ex(loader.GetIndicesCount(0));

			loader.ExtractPositionData(0, sizeof(VertexPC), 0, &vertices[0]);
			loader.ExtractIndexData(0, sizeof(s32), 0, &indices[0]);
			for (u32 i = 0; i < vertices.get_size(); i++)
			{
				vertices[i].Color = floral::vec4f(1.0f);
			}

			{
				insigne::vbdesc_t desc;
				desc.region_size = vertices.get_size() * sizeof(VertexPC);
				desc.stride = sizeof(VertexPC);
				desc.data = nullptr;
				desc.count = 0;
				desc.usage = insigne::buffer_usage_e::dynamic_draw;

				insigne::vb_handle_t newVB = insigne::create_vb(desc);
				insigne::copy_update_vb(newVB, &vertices[0], vertices.get_size(), sizeof(VertexPC), 0);
				m_VBs.push_back(newVB);
			}

			{
				insigne::ibdesc_t desc;
				desc.region_size = indices.get_size() * sizeof(s32);
				desc.data = nullptr;
				desc.count = 0;
				desc.usage = insigne::buffer_usage_e::dynamic_draw;

				insigne::ib_handle_t newIB = insigne::create_ib(desc);
				insigne::copy_update_ib(newIB, &indices[0], indices.get_size(), 0);
				m_IBs.push_back(newIB);
			}

			if (i % 5 == 0)
			{
				insigne::dispatch_render_pass();
			}
		}
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_SceneData.WVP = m_CameraMotion.GetWVP();
		m_SceneData.XForm = floral::mat4x4f(1.0f);

		insigne::update_ub(newUB, &m_SceneData, sizeof(SceneData), 0);
		m_UB = newUB;
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/internal/cornel_box_vs");
		desc.fs_path = floral::path("/internal/cornel_box_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		// static uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Scene");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		}
	}

	insigne::dispatch_render_pass();
}

void CbFormats::OnUpdate(const f32 i_deltaMs)
{
	m_CameraMotion.OnUpdate(i_deltaMs);
}

void CbFormats::OnDebugUIUpdate(const f32 i_deltaMs)
{
}

void CbFormats::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	for (u32 i = 0; i < m_VBs.get_size(); i++)
	{
		insigne::draw_surface<SurfacePC>(m_VBs[i], m_IBs[i], m_Material);
	}
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void CbFormats::OnCleanUp()
{
}

}
