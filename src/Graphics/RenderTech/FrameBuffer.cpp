#include "FrameBuffer.h"

#include <floral/containers/array.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include <calyx/context.h>

#include "InsigneImGui.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone
{
namespace tech
{

static const_cstr k_SuiteName = "framebuffer";

//----------------------------------------------

static const_cstr s_VertexShaderCode = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_VertColor;

out vec4 o_VertColor;

void main() {
	o_VertColor = l_VertColor;
	gl_Position = vec4(l_Position_L, 1.0f);
}
)";

static const_cstr s_FragmentShaderCode = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 o_VertColor;

void main()
{
	o_Color = o_VertColor;
}
)";

static const_cstr s_GrayScaleVS = R"(#version 300 es
layout (location = 0) in highp vec2 l_Position;
layout (location = 1) in mediump vec2 l_TexCoord;

out mediump vec2 v_TexCoord;

void main()
{
	v_TexCoord = l_TexCoord;
	gl_Position = vec4(l_Position, 0.0f, 1.0f);
}
)";

static const_cstr s_GrayScaleFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D u_Tex;

in mediump vec2 v_TexCoord;

void main()
{
	mediump vec3 color = texture(u_Tex, v_TexCoord).rgb;
	// linear luminance
	mediump float luminance = 0.2126f * color.r +  0.7152f * color.g + 0.0722f * color.b;
	o_Color = vec4(luminance, luminance, luminance, 1.0f);
}
)";

//----------------------------------------------

FrameBuffer::FrameBuffer()
{
}

FrameBuffer::~FrameBuffer()
{
}

const_cstr FrameBuffer::GetName() const
{
	return k_SuiteName;
}

void FrameBuffer::OnInitialize(floral::filesystem<FreelistArena>* i_fs)
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_SuiteName);
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	// register surfaces
	insigne::register_surface_type<SurfacePC>();
	insigne::register_surface_type<SurfacePT>();

	floral::inplace_array<VertexPC, 3> vertices;
	vertices.push_back(VertexPC { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } });
	vertices.push_back(VertexPC { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } });
	vertices.push_back(VertexPC { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } });

	floral::inplace_array<s32, 3> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(2);
		desc.stride = sizeof(VertexPC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;
		
		m_VB = insigne::create_vb(desc);
		insigne::copy_update_vb(m_VB, &vertices[0], vertices.get_size(), sizeof(VertexPC), 0);
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

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();

		strcpy(desc.vs, s_VertexShaderCode);
		strcpy(desc.fs, s_FragmentShaderCode);
		desc.vs_path = floral::path("/demo/simple_vs");
		desc.fs_path = floral::path("/demo/simple_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);
	}

	// framebuffer
	{
		calyx::context_attribs* commonCtx = calyx::get_context_attribs();

		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		desc.color_attachments->push_back(insigne::color_attachment_t("main_color", insigne::texture_format_e::rgba));

		desc.width = commonCtx->window_width;
		desc.height = commonCtx->window_height;

		m_PostFXBuffer = insigne::create_framebuffer(desc);
	}
	
	floral::inplace_array<VertexPT, 4> ssVertices;
	ssVertices.push_back(VertexPT { { -1.0f, -1.0f }, { 0.0f, 0.0f } });
	ssVertices.push_back(VertexPT { { 1.0f, -1.0f }, { 1.0f, 0.0f } });
	ssVertices.push_back(VertexPT { { 1.0f, 1.0f }, { 1.0f, 1.0f } });
	ssVertices.push_back(VertexPT { { -1.0f, 1.0f }, { 0.0f, 1.0f } });

	floral::inplace_array<s32, 6> ssIndices;
	ssIndices.push_back(0);
	ssIndices.push_back(1);
	ssIndices.push_back(2);
	ssIndices.push_back(2);
	ssIndices.push_back(3);
	ssIndices.push_back(0);

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(2);
		desc.stride = sizeof(VertexPT);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;
		
		m_SSVB = insigne::create_vb(desc);
		insigne::copy_update_vb(m_SSVB, &ssVertices[0], ssVertices.get_size(), sizeof(VertexPT), 0);
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(1);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_SSIB = insigne::create_ib(desc);
		insigne::copy_update_ib(m_SSIB, &ssIndices[0], ssIndices.get_size(), 0);
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_GrayScaleVS);
		strcpy(desc.fs, s_GrayScaleFS);
		desc.vs_path = floral::path("/demo/grayscale_vs");
		desc.fs_path = floral::path("/demo/grayscale_fs");

		m_GrayScaleShader = insigne::create_shader(desc);
		insigne::infuse_material(m_GrayScaleShader, m_GrayScaleMaterial);

		{
			ssize texSlot = insigne::get_material_texture_slot(m_GrayScaleMaterial, "u_Tex");
			m_GrayScaleMaterial.textures[texSlot].value = insigne::extract_color_attachment(m_PostFXBuffer, 0);
		}
	}
}

void FrameBuffer::OnUpdate(const f32 i_deltaMs)
{
}

void FrameBuffer::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(m_PostFXBuffer);
	insigne::draw_surface<SurfacePC>(m_VB, m_IB, m_Material);
	insigne::end_render_pass(m_PostFXBuffer);
	insigne::dispatch_render_pass();

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::draw_surface<SurfacePT>(m_SSVB, m_SSIB, m_GrayScaleMaterial);
	RenderImGui();
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void FrameBuffer::OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_SuiteName);

	insigne::unregister_surface_type<SurfacePT>();
	insigne::unregister_surface_type<SurfacePC>();

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

}
}
