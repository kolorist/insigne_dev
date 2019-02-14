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

CbFormats::CbFormats()
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
	m_PlyReaderArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
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

			insigne::vbdesc_t desc;
			desc.region_size = SIZE_KB(64);
			desc.stride = sizeof(VertexPC);
			desc.data = nullptr;
			desc.count = 0;
			desc.usage = insigne::buffer_usage_e::dynamic_draw;

			insigne::vb_handle_t newVB = insigne::create_vb(desc);
			m_VBs.push_back(newVB);

			if (i % 5 == 0)
			{
				insigne::dispatch_render_pass();
			}
		}
	}

	insigne::dispatch_render_pass();
}

void CbFormats::OnUpdate(const f32 i_deltaMs)
{
}

void CbFormats::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void CbFormats::OnCleanUp()
{
}

}
