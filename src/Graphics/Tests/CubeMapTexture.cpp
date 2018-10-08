#include "CubeMapTexture.h"

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

namespace stone {

static const_cstr s_VertexShader = R"(
#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_XForm
{
	highp mat4 iu_WVP;
};

out mediump vec3 v_SampleDir_W;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_SampleDir_W = pos_W.xyz;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(
#version 300 es

layout (location = 0) out mediump vec4 o_Color;

uniform mediump samplerCube u_Tex;
in mediump vec3 v_SampleDir_W;

void main()
{
	mediump vec3 sampleDir = normalize(v_SampleDir_W);
	mediump vec3 outColor = texture(u_Tex, sampleDir).rgb;
	o_Color = vec4(outColor, 1.0f);
}
)";

static const_cstr s_DebugVertexShader = R"(
#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

layout(std140) uniform ub_XForm
{
	highp mat4 iu_WVP;
};

layout(std140) uniform ub_SSData
{
	highp vec2 iu_SSPosition;
};

out mediump vec4 v_Color;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_Color = l_Color;
	highp vec4 pos = iu_WVP * pos_W;
	gl_Position = pos + vec4(iu_SSPosition.x * pos.w, iu_SSPosition.y * pos.w, 0.0f, 0.0f);
}
)";

static const_cstr s_DebugFragmentShader = R"(
#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_Color;

void main()
{
	o_Color = v_Color;
}
)";

CubeMapTexture::CubeMapTexture()
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(48));
}

CubeMapTexture::~CubeMapTexture()
{
}

void CubeMapTexture::OnInitialize()
{
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(DemoVertex);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);

		m_Vertices.init(8u, &g_StreammingAllocator);
		m_Vertices.push_back({ floral::vec3f(-1.0f, -1.0f, 1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(1.0f, -1.0f, 1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(1.0f, -1.0f, -1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(-1.0f, -1.0f, -1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(-1.0f, 1.0f, 1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(1.0f, 1.0f, 1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(1.0f, 1.0f, -1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(-1.0f, 1.0f, -1.0f), floral::vec4f(0.0f) });

		insigne::update_vb(newVB, &m_Vertices[0], 8, 0);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);

		m_Indices.init(36u, &g_StreammingAllocator);
		m_Indices.push_back(0);
		m_Indices.push_back(1);
		m_Indices.push_back(2);
		m_Indices.push_back(2);
		m_Indices.push_back(3);
		m_Indices.push_back(0);

		m_Indices.push_back(4);
		m_Indices.push_back(5);
		m_Indices.push_back(6);
		m_Indices.push_back(6);
		m_Indices.push_back(7);
		m_Indices.push_back(4);

		m_Indices.push_back(3);
		m_Indices.push_back(0);
		m_Indices.push_back(4);
		m_Indices.push_back(4);
		m_Indices.push_back(7);
		m_Indices.push_back(3);

		m_Indices.push_back(5);
		m_Indices.push_back(1);
		m_Indices.push_back(2);
		m_Indices.push_back(2);
		m_Indices.push_back(6);
		m_Indices.push_back(5);

		m_Indices.push_back(4);
		m_Indices.push_back(0);
		m_Indices.push_back(1);
		m_Indices.push_back(1);
		m_Indices.push_back(5);
		m_Indices.push_back(4);

		m_Indices.push_back(6);
		m_Indices.push_back(2);
		m_Indices.push_back(3);
		m_Indices.push_back(3);
		m_Indices.push_back(7);
		m_Indices.push_back(6);

		insigne::update_ib(newIB, &m_Indices[0], m_Indices.get_size(), 0);
		m_IB = newIB;
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_CamView.position = floral::vec3f(5.0f, 5.0f, 5.0f);
		m_CamView.look_at = floral::vec3f(0.0f);
		m_CamView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

		m_CamProj.near_plane = 0.01f; m_CamProj.far_plane = 100.0f;
		m_CamProj.fov = 60.0f;
		m_CamProj.aspect_ratio = 16.0f / 9.0f;

		m_Data.WVP = floral::construct_perspective(m_CamProj) * construct_lookat_point(m_CamView);

		insigne::update_ub(newUB, &m_Data, sizeof(MyData), 0);
		m_UB = newUB;
	}

	{
		floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/alexs_apartment.cbskb");
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
		demoTexDesc.format = insigne::texture_format_e::hdr_rgb;
		demoTexDesc.min_filter = insigne::filtering_e::linear_mipmap_linear;
		demoTexDesc.mag_filter = insigne::filtering_e::linear;
		demoTexDesc.dimension = insigne::texture_dimension_e::tex_cube;
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

	// cube shader
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_XForm", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler_cube));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_XForm");
			m_Material.uniform_blocks[ubSlot].value = m_UB;
		}
		{
			s32 texSlot = insigne::get_material_texture_slot(m_Material, "u_Tex");
			m_Material.textures[texSlot].value = m_Texture;
		}
	}

	// debug draw
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(DebugVertex);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);

		m_DebugVertices.init(8u, &g_StreammingAllocator);
		// x
		m_DebugVertices.push_back({ floral::vec3f(0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f) });
		m_DebugVertices.push_back({ floral::vec3f(1.0f, 0.0f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f) });
		// y
		m_DebugVertices.push_back({ floral::vec3f(0.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f) });
		m_DebugVertices.push_back({ floral::vec3f(0.0f, 1.0f, 0.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f) });
		// z
		m_DebugVertices.push_back({ floral::vec3f(0.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f) });
		m_DebugVertices.push_back({ floral::vec3f(0.0f, 0.0f, 1.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f) });

		insigne::update_vb(newVB, &m_DebugVertices[0], m_DebugVertices.get_size(), 0);
		m_DebugVB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);

		m_DebugIndices.init(36u, &g_StreammingAllocator);
		m_DebugIndices.push_back(0);
		m_DebugIndices.push_back(1);
		m_DebugIndices.push_back(2);
		m_DebugIndices.push_back(3);
		m_DebugIndices.push_back(4);
		m_DebugIndices.push_back(5);

		insigne::update_ib(newIB, &m_DebugIndices[0], m_DebugIndices.get_size(), 0);
		m_DebugIB = newIB;
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		m_SSData.SSPosition = floral::vec2f(-0.7f, -0.7f);

		insigne::update_ub(newUB, &m_SSData, sizeof(MySSData), 0);
		m_DebugUB = newUB;
	}

	// debug shader
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_XForm", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_SSData", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_DebugVertexShader);
		strcpy(desc.fs, s_DebugFragmentShader);

		m_DebugShader = insigne::create_shader(desc);
		insigne::infuse_material(m_DebugShader, m_DebugMaterial);

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_DebugMaterial, "ub_XForm");
			m_DebugMaterial.uniform_blocks[ubSlot].value = m_UB;
		}
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_DebugMaterial, "ub_SSData");
			m_DebugMaterial.uniform_blocks[ubSlot].value = m_DebugUB;
		}
	}

	// flush the initialization pass
	insigne::dispatch_render_pass();
}

void CubeMapTexture::OnUpdate(const f32 i_deltaMs)
{
}

void CubeMapTexture::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	// render here
	insigne::draw_surface<DemoSurface>(m_VB, m_IB, m_Material);
	insigne::draw_surface<DebugLine>(m_DebugVB, m_DebugIB, m_DebugMaterial);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void CubeMapTexture::OnCleanUp()
{
}

}
