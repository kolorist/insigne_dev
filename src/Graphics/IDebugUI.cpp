#include "IDebugUI.h"

#include <context.h>

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
	o_VertColor = l_VertColor;
	gl_Position = iu_DebugOrthoWVP * vec4(l_Position_L.xy, 0.0, 1.0f);
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D iu_Tex;

in mediump vec2 o_TexCoord;
in mediump vec4 o_VertColor;

void main()
{
	o_Color = o_VertColor * texture(iu_Tex, o_TexCoord.st);
}
)";

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
	insigne::texture_handle_t uiTexHandle = insigne::create_texture(uiTexDesc);

	io.Fonts->TexID = (voidptr)(size)uiTexHandle;
	io.RenderDrawListsFn = &IDebugUI::RenderImGuiDrawLists;

	// stream data
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(VertexPC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		m_IB = newIB;
	}

	// shader
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_XForm", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);
	}

	insigne::dispatch_render_pass();

#if 0
	insigne::texture_handle_t uiTex = m_TextureManager->CreateTexture(pixels, width, height,
			insigne::texture_format_e::rgba);
	io.Fonts->TexID = (voidptr)(size)uiTex;
	io.RenderDrawListsFn = &Debugger::RenderImGuiDrawLists;

	// surfaces
	s_UISurface = insigne::create_streamed_surface(sizeof(ImDrawVert));

	// setup material
	s_UIMaterial = m_MaterialManager->CreateMaterial<DebugUIMaterial>("shaders/internal/debugui");
	s_UIMaterial->SetTexture(uiTex);
	floral::mat4x4f debugOrtho(
			floral::vec4f(2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f),
			floral::vec4f(0.0f, -2.0f / io.DisplaySize.y, 0.0f, 0.0f),
			floral::vec4f(0.0f, 0.0f, -1.0f, 0.0f),
			floral::vec4f(-1.0f, 1.0f, 0.0f, 1.0f));
	s_UIMaterial->SetDebugOrthoWVP(debugOrtho);
#endif
}

void IDebugUI::OnFrameUpdate(const f32 i_deltaMs)
{
	ImGuiIO& io = ImGui::GetIO();

	io.DeltaTime = i_deltaMs / 1000.0f;
	ImGui::NewFrame();

	ImGui::Begin("test");
	ImGui::Text("abc");
	ImGui::End();

	OnDebugUIUpdate(i_deltaMs);
}

void IDebugUI::OnFrameRender(const f32 i_deltaMs)
{
	ImGui::Render();
}

//----------------------------------------------
void IDebugUI::RenderImGuiDrawLists(ImDrawData* i_drawData)
{
	ImGuiIO& io = ImGui::GetIO();
	s32 fbWidth = (s32)(io.DisplaySize.x * 1.0f); //io.DisplayFramebufferScale.x);
	s32 fbHeight = (s32)(io.DisplaySize.y * 1.0f); //io.DisplayFramebufferScale.y);

	i_drawData->ScaleClipRects(ImVec2(1.0f, 1.0f));

	for (s32 i = 0; i < i_drawData->CmdListsCount; i++)
	{
		const ImDrawList* cmdList = i_drawData->CmdLists[i];
		const ImDrawIdx* idxBufferOffset = 0;
		for (s32 cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++)
		{
			const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[cmdIdx];
			if (drawCmd->UserCallback)
			{
			}
			else
			{
			}
			idxBufferOffset += drawCmd->ElemCount;
		}
	}
#if 0
	for (s32 i = 0; i < i_drawData->CmdListsCount; i++) {
		const ImDrawList* cmdList = i_drawData->CmdLists[i];
		const ImDrawIdx* idxBufferOffset = 0;
		insigne::update_streamed_surface(s_UISurface,
				(voidptr)cmdList->VtxBuffer.Data, (size)cmdList->VtxBuffer.Size * sizeof(ImDrawVert),
				(voidptr)cmdList->IdxBuffer.Data, (size)cmdList->IdxBuffer.Size * sizeof(ImDrawIdx),
				cmdList->VtxBuffer.Size, cmdList->IdxBuffer.Size);
		for (s32 cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++) {
			const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[cmdIdx];
			if (drawCmd->UserCallback) {
				drawCmd->UserCallback(cmdList, drawCmd);
			} else {
				s32 x0 = (s32)drawCmd->ClipRect.x;	// topleft
				s32 y0 = (s32)drawCmd->ClipRect.y;
				s32 w = (s32)(drawCmd->ClipRect.z - drawCmd->ClipRect.x);
				s32 h = (s32)(drawCmd->ClipRect.w - drawCmd->ClipRect.y);

				// lower left
				y0 = fbHeight - (y0 + h);
				insigne::set_scissor_test<ImGuiSurface>(
						true, x0, y0, w, h);
				insigne::draw_surface_segmented<ImGuiSurface>(s_UISurface,
					s_UIMaterial->GetHandle(),
					(s32)drawCmd->ElemCount, (voidptr)idxBufferOffset);
			}
			idxBufferOffset += drawCmd->ElemCount;
		}
	}
#endif
}

}
