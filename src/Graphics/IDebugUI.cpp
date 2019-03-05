#include "IDebugUI.h"

#include <context.h>
#include <clover.h>

#include <insigne/ut_textures.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

namespace stone
{

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec2 l_Position_L;
layout (location = 1) in mediump vec2 l_TexCoord;
layout (location = 2) in mediump vec4 l_VertColor;

layout(std140) uniform ub_XForm
{
	highp mat4 iu_DebugOrthoWVP;
};

out vec2 o_TexCoord;
out vec4 o_VertColor;

void main() {
	o_TexCoord = l_TexCoord;
	o_VertColor = l_VertColor / 255.0f;
	gl_Position = iu_DebugOrthoWVP * vec4(l_Position_L.xy, 0.0, 1.0f);
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D u_Tex;

in mediump vec2 o_TexCoord;
in mediump vec4 o_VertColor;

void main()
{
	o_Color = o_VertColor * texture(u_Tex, o_TexCoord.st);
}
)";

IDebugUI::IDebugUI()
	: m_CursorPressed(false)
	, m_CursorHeldThisFrame(false)
	, m_ShowDebugMenu(false)
	, m_ShowDebugInfo(false)
	, m_ShowInsigneInfo(false)
{
}

void IDebugUI::Initialize()
{
	ImGuiIO& io = ImGui::GetIO();

	// display size
	io.DisplaySize = ImVec2(
			(f32)calyx::g_context_attribs->window_width,
			(f32)calyx::g_context_attribs->window_height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	// fonts
	u8* pixels;
	s32 width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	insigne::texture_desc_t uiTexDesc;
	uiTexDesc.width = width;
	uiTexDesc.height = height;
	uiTexDesc.format = insigne::texture_format_e::rgba;
	uiTexDesc.min_filter = insigne::filtering_e::nearest;
	uiTexDesc.mag_filter = insigne::filtering_e::nearest;
	uiTexDesc.dimension = insigne::texture_dimension_e::tex_2d;
	uiTexDesc.has_mipmap = false;

	const size dataSize = insigne::prepare_texture_desc(uiTexDesc);

	// TODO: memcpy? really?
	memcpy(uiTexDesc.data, pixels, dataSize);
	m_Texture = insigne::create_texture(uiTexDesc);

	io.Fonts->TexID = (voidptr)m_Texture;
	io.RenderDrawListsFn = nullptr;

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);
		m_UB = newUB;

		floral::mat4x4f debugOrtho(
				floral::vec4f(2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f),
				floral::vec4f(0.0f, -2.0f / io.DisplaySize.y, 0.0f, 0.0f),
				floral::vec4f(0.0f, 0.0f, -1.0f, 0.0f),
				floral::vec4f(-1.0f, 1.0f, 0.0f, 1.0f));
		insigne::copy_update_ub(m_UB, &debugOrtho, sizeof(floral::mat4x4f), 0);
	}

	// shader
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_XForm", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_XForm");
		m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		s32 texSlot = insigne::get_material_texture_slot(m_Material, "u_Tex");
		m_Material.textures[texSlot].value = m_Texture;
	}

	insigne::dispatch_render_pass();
}

void IDebugUI::OnFrameUpdate(const f32 i_deltaMs)
{
	ImGuiIO& io = ImGui::GetIO();

	io.MousePos = ImVec2(m_CursorPos.x, m_CursorPos.y);
	io.MouseDown[0] = m_CursorHeldThisFrame | m_CursorPressed;
	m_CursorHeldThisFrame = false;

	io.DeltaTime = i_deltaMs / 1000.0f;
	ImGui::NewFrame();

	//ImGui::ShowTestWindow();
	if (m_ShowDebugMenu)
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("TestSuite"))
			{
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Insigne"))
			{
				ImGui::MenuItem("Debug UI Info", NULL, &m_ShowDebugInfo);
				ImGui::MenuItem("Insigne Info", NULL, &m_ShowInsigneInfo);
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	}

	if (m_ShowDebugInfo)
	{
		if (ImGui::Begin("Debug UI Information"))
		{
			ImGui::End();
		}
	}

	if (m_ShowInsigneInfo)
	{
	}

	OnDebugUIUpdate(i_deltaMs);
}

void IDebugUI::OnFrameRender(const f32 i_deltaMs)
{
	ImGui::Render();
	RenderImGui(ImGui::GetDrawData());
}

//----------------------------------------------
void IDebugUI::RenderImGui(ImDrawData* i_drawData)
{
	ImGuiIO& io = ImGui::GetIO();
	s32 fbWidth = (s32)(io.DisplaySize.x * 1.0f); //io.DisplayFramebufferScale.x);
	s32 fbHeight = (s32)(io.DisplaySize.y * 1.0f); //io.DisplayFramebufferScale.y);

	i_drawData->ScaleClipRects(ImVec2(1.0f, 1.0f));

	for (s32 i = 0; i < i_drawData->CmdListsCount; i++)
	{
		s32 bufferSlot = -1;
		if (i > ((s32)m_IBs.get_size() - 1))
		{
			bufferSlot = AllocateNewBuffers();
		}
		else
		{
			bufferSlot = i;
		}
		const ImDrawList* cmdList = i_drawData->CmdLists[i];
		insigne::copy_update_vb(m_VBs[bufferSlot], (voidptr)cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size,
				sizeof(ImDrawVert), 0);
		insigne::copy_update_ib(m_IBs[bufferSlot], (voidptr)cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size, 0);
		s32 idxBufferOffset = 0;
		for (s32 cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++)
		{
			const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[cmdIdx];
			s32 x0 = (s32)drawCmd->ClipRect.x;	// topleft
			s32 y0 = fbHeight - (s32)drawCmd->ClipRect.w;
			s32 w = (s32)(drawCmd->ClipRect.z - drawCmd->ClipRect.x);
			s32 h = (s32)(drawCmd->ClipRect.w - drawCmd->ClipRect.y);

			insigne::setup_scissor<ImGuiSurface>(true, x0, y0, w, h);
			insigne::draw_surface<ImGuiSurface>(m_VBs[bufferSlot], m_IBs[bufferSlot], m_Material,
					idxBufferOffset, (s32)drawCmd->ElemCount);
			idxBufferOffset += drawCmd->ElemCount;
		}
	}
}

// ---------------------------------------------

const s32 IDebugUI::AllocateNewBuffers()
{
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(128);
		desc.stride = sizeof(ImGuiVertex);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		m_VBs.push_back(newVB);
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		m_IBs.push_back(newIB);
	}
	return (m_IBs.get_size() - 1);
}

// ---------------------------------------------
void IDebugUI::OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus)
{
	if (i_keyCode == 0x70 && i_keyStatus == 0) // F1
	{
		m_ShowDebugMenu = !m_ShowDebugMenu;
	}
}

void IDebugUI::OnCursorMove(const u32 i_x, const u32 i_y)
{
	m_CursorPos = floral::vec2f(i_x, i_y);
}

void IDebugUI::OnCursorInteract(const bool i_pressed)
{
	m_CursorPressed = i_pressed;
	if (i_pressed)
	{
		m_CursorHeldThisFrame = true;
	}
}

}
