#include "PlainTextureQuad.h"

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

namespace stone {

static const_cstr s_VertexShader = R"(
#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec2 l_TexCoord;

out mediump vec2 v_TexCoord;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_TexCoord = l_TexCoord;
	gl_Position = pos_W;
}
)";

static const_cstr s_FragmentShader = R"(
#version 300 es

in mediump vec2 v_TexCoord;

layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Data
{
	mediump vec4 iu_Color;
};

uniform mediump sampler2D u_Tex;

void main()
{
	o_Color = vec4(texture(u_Tex, v_TexCoord).rgb, 1.0f);
}
)";

PlainTextureQuad::PlainTextureQuad()
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(4));
}

PlainTextureQuad::~PlainTextureQuad()
{
}

void PlainTextureQuad::OnInitialize()
{
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(DemoVertex);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);

		m_Vertices.init(4u, &g_StreammingAllocator);
		m_Vertices.push_back({ floral::vec3f(-0.5f, -0.5f, 0.0f), floral::vec2f(0.0f, 0.0f) });
		m_Vertices.push_back({ floral::vec3f(0.5f, -0.5f, 0.0f), floral::vec2f(1.0f, 0.0f) });
		m_Vertices.push_back({ floral::vec3f(0.5f, 0.5f, 0.0f), floral::vec2f(1.0f, 1.0f) });
		m_Vertices.push_back({ floral::vec3f(-0.5f, 0.5f, 0.0f), floral::vec2f(0.0f, 1.0f) });

		insigne::update_vb(newVB, &m_Vertices[0], 4, 0);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);

		m_Indices.init(6u, &g_StreammingAllocator);
		m_Indices.push_back(0);
		m_Indices.push_back(1);
		m_Indices.push_back(2);
		m_Indices.push_back(2);
		m_Indices.push_back(3);
		m_Indices.push_back(0);

		insigne::update_ib(newIB, &m_Indices[0], 6, 0);
		m_IB = newIB;
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		m_Data.Color = floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f);

		insigne::update_ub(newUB, &m_Data, sizeof(MyData), 0);
		m_UB = newUB;
	}

	{
		floral::file_info texFile = floral::open_file("gfx/go/textures/demo/uvchecker2.cbtex");
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);
		floral::close_file(texFile);

		c8 magicChars[4];
		dataStream.read_bytes(magicChars, 4);

		s32 colorRange = 0;
		s32 colorSpace = 0;
		s32 colorChannel = 0;
		f32 encodeGamma = 0.0f;
		s32 mipsCount = 0;
		dataStream.read<s32>(&colorRange);
		dataStream.read<s32>(&colorSpace);
		dataStream.read<s32>(&colorChannel);
		dataStream.read<f32>(&encodeGamma);
		dataStream.read<s32>(&mipsCount);	

		insigne::texture_desc_t demoTexDesc;
		demoTexDesc.width = 512;
		demoTexDesc.height = 512;
		demoTexDesc.format = insigne::texture_format_e::rgb;
		demoTexDesc.min_filter = insigne::filtering_e::linear_mipmap_linear;
		demoTexDesc.mag_filter = insigne::filtering_e::linear;
		demoTexDesc.dimension = insigne::texture_dimension_e::tex_2d;
		demoTexDesc.has_mipmap = true;
		const size dataSize = insigne::prepare_texture_desc(demoTexDesc);
		p8 pData = (p8)demoTexDesc.data;
		// > This is where it get interesting
		// > When displaying image in the screen, the usual coordinate origin for us is in upper left corner
		// 	and the data stored in disk *may* in scanlines from top to bottom
		// > But the texture coordinate origin of OpenGL starts from bottom left corner
		// 	and the data stored will be read and displayed in a order from bottom to top
		// Thus, we have to store our texture data in disk in the order of bottom to top scanlines
		dataStream.read_bytes((p8)demoTexDesc.data, dataSize);

		m_Texture = insigne::create_texture(demoTexDesc);

		m_MemoryArena->free_all();
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Data", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Data");
		m_Material.uniform_blocks[ubSlot].value = m_UB;
		s32 texSlot = insigne::get_material_texture_slot(m_Material, "u_Tex");
		m_Material.textures[texSlot].value = m_Texture;
	}

	// flush the initialization pass
	insigne::dispatch_render_pass();
}

void PlainTextureQuad::OnUpdate(const f32 i_deltaMs)
{
}

void PlainTextureQuad::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	// render here
	insigne::draw_surface<DemoTexturedSurface>(m_VB, m_IB, m_Material);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void PlainTextureQuad::OnCleanUp()
{
}

}
