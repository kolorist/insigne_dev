#include "ImDrawer2D.h"

#include <floral/math/utils.h>

#include <clover/logger.h>

#include <insigne/system.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

namespace stone
{
// ----------------------------------------------------------------------------

static const_cstr s_VS = R"(#version 300 es
layout (location = 0) in highp vec2 l_Position_L;
layout (location = 1) in mediump vec2 l_TexCoord;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_XForm
{
	highp mat4 iu_WVP;
};

out mediump vec2 v_TexCoord;
out mediump vec4 v_Color;

void main() {
	v_Color = l_Color;
	v_TexCoord = l_TexCoord;
	highp vec4 pos = iu_WVP * vec4(l_Position_L, 0.0f, 1.0f);
	gl_Position = pos;
}
)";

static const_cstr s_FS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 v_TexCoord;
in mediump vec4 v_Color;

uniform mediump sampler2D u_Tex;

void main()
{
    o_Color = v_Color * texture(u_Tex, v_TexCoord);
}
)";

// ----------------------------------------------------------------------------

ImDrawer2D::ImDrawer2D()
{
}

// ----------------------------------------------------------------------------

ImDrawer2D::~ImDrawer2D()
{
}

// ----------------------------------------------------------------------------

void ImDrawer2D::Initialize(const InitializeData& i_initData)
{
    m_MemoryArena = i_initData.memoryArena;

    {
        insigne::ubdesc_t desc;
        desc.region_size = floral::next_pow2(sizeof(XFormData));
        desc.data = nullptr;
        desc.data_size = 0;
        desc.usage = insigne::buffer_usage_e::dynamic_draw;

        m_UB = insigne::create_ub(desc);
    }

    {
        insigne::shader_desc_t desc = insigne::create_shader_desc();
        desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_XForm", insigne::param_data_type_e::param_ub));

        strcpy(desc.vs, s_VS);
        strcpy(desc.fs, s_FS);
        desc.vs_path = floral::path("/internal/im2d_vs");
        desc.fs_path = floral::path("/internal/im2d_fs");

        m_Shader = insigne::create_shader(desc);
        insigne::infuse_material(m_Shader, m_Material);

        s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_XForm");
        m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
        m_Material.render_state.depth_write = false;
        m_Material.render_state.depth_test = true;
        m_Material.render_state.depth_func = insigne::compare_func_e::func_less_or_equal;
        m_Material.render_state.cull_face = false;
        m_Material.render_state.blending = true;
        m_Material.render_state.blend_equation = insigne::blend_equation_e::func_add;
        m_Material.render_state.blend_func_sfactor = insigne::factor_e::fact_src_alpha;
        m_Material.render_state.blend_func_dfactor = insigne::factor_e::fact_one_minus_src_alpha;
    }

	insigne::dispatch_render_pass();
    CLOVER_DEBUG("ImDrawer2D initialized.");
}

// ----------------------------------------------------------------------------
}
