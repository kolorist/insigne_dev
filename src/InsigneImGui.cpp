#include "InsigneImGui.h"

#include <clover/Logger.h>

#include <imgui.h>

#include <calyx/context.h>

#include <insigne/counters.h>
#include <insigne/ut_textures.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/memory.h>
#include <insigne/driver.h>

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"

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

//----------------------------------------------

static const size k_CPUMemoryBudget = SIZE_MB(4);
static const size k_VertexBufferBudget = SIZE_MB(4);
static const size k_IndexBufferBudget = SIZE_MB(2);

static floral::inplace_array<insigne::vb_handle_t, 8> s_ImGuiVB;
static floral::inplace_array<insigne::ib_handle_t, 8> s_ImGuiIB;
static insigne::ub_handle_t s_ImGuiUB;
static insigne::shader_handle_t s_ImGuiShader;
static insigne::texture_handle_t s_ImGuiTexture;
static insigne::material_desc_t s_ImGuiMaterial;

static floral::vec2f s_CursorPos;
static bool s_CursorPressed;
static bool s_CursorHeldThisFrame;

static FreelistArena* s_MemoryArena = nullptr;

//----------------------------------------------

const ssize AllocateNewBuffer();

static inline void* ImGuiCustomAlloc(const size_t sz)
{
	return s_MemoryArena->allocate(sz);
}

static inline void ImGuiCustomFree(void* ptr)
{
	if (ptr)
	{
		s_MemoryArena->free(ptr);
	}
}

void InitializeImGui()
{
	FLORAL_ASSERT(s_MemoryArena == nullptr);
	s_MemoryArena = g_PersistanceAllocator.allocate_arena<FreelistArena>(k_CPUMemoryBudget);

	ImGuiIO& io = ImGui::GetIO();
	io.MemAllocFn = &ImGuiCustomAlloc;
	io.MemFreeFn = &ImGuiCustomFree;

	calyx::context_attribs* commonCtx = calyx::get_context_attribs();

	// display size
	io.DisplaySize = ImVec2(
			(f32)commonCtx->window_width,
			(f32)commonCtx->window_height);
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
	s_ImGuiTexture = insigne::create_texture(uiTexDesc);
	io.Fonts->TexID = &s_ImGuiTexture;

	io.RenderDrawListsFn = nullptr;

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);
		s_ImGuiUB = newUB;

		floral::mat4x4f debugOrtho(
				floral::vec4f(2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f),
				floral::vec4f(0.0f, -2.0f / io.DisplaySize.y, 0.0f, 0.0f),
				floral::vec4f(0.0f, 0.0f, -1.0f, 0.0f),
				floral::vec4f(-1.0f, 1.0f, 0.0f, 1.0f));
		insigne::copy_update_ub(s_ImGuiUB, &debugOrtho, sizeof(floral::mat4x4f), 0);
	}

	// shader
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_XForm", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/internal/debugui_vs");
		desc.fs_path = floral::path("/internal/debugui_fs");

		s_ImGuiShader = insigne::create_shader(desc);
		insigne::infuse_material(s_ImGuiShader, s_ImGuiMaterial);

		s32 ubSlot = insigne::get_material_uniform_block_slot(s_ImGuiMaterial, "ub_XForm");
		s_ImGuiMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, s_ImGuiUB };
		s32 texSlot = insigne::get_material_texture_slot(s_ImGuiMaterial, "u_Tex");
		s_ImGuiMaterial.textures[texSlot].value = s_ImGuiTexture;
	}

	for (ssize i = 0; i < 8; i++)
	{
		AllocateNewBuffer();
	}
	insigne::dispatch_render_pass();
}

void UpdateImGui()
{
	ImGuiIO& io = ImGui::GetIO();

	io.MousePos = ImVec2(s_CursorPos.x, s_CursorPos.y);
	io.MouseDown[0] = s_CursorHeldThisFrame | s_CursorPressed;
	s_CursorHeldThisFrame = false;

	io.DeltaTime = 16.6667f / 1000.0f;
}

static const ssize AllocateNewBuffer()
{
	{
		insigne::vbdesc_t desc;
		desc.region_size = k_VertexBufferBudget;
		desc.stride = sizeof(ImGuiVertex);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		s_ImGuiVB.push_back(newVB);
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = k_IndexBufferBudget;
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		s_ImGuiIB.push_back(newIB);
	}
	return (s_ImGuiIB.get_size() - 1);
}

void RenderImGui()
{
	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	FLORAL_ASSERT(drawData != nullptr);

	ImGuiIO& io = ImGui::GetIO();
	s32 fbWidth = (s32)(io.DisplaySize.x * 1.0f); //io.DisplayFramebufferScale.x);
	s32 fbHeight = (s32)(io.DisplaySize.y * 1.0f); //io.DisplayFramebufferScale.y);

	drawData->ScaleClipRects(ImVec2(1.0f, 1.0f));

	//m_UsedVertexMemory = 0;
	//m_UsedIndexMemory = 0;
	for (s32 i = 0; i < drawData->CmdListsCount; i++)
	{
		ssize bufferSlot = -1;
		if (i > (s_ImGuiIB.get_size() - 1))
		{
			FLORAL_ASSERT_MSG(false, "Dangerous!!! May cause corrupted render when cleaning up a suite");
			bufferSlot = AllocateNewBuffer();
		}
		else
		{
			bufferSlot = i;
		}
		const ImDrawList* cmdList = drawData->CmdLists[i];
		insigne::copy_update_vb(s_ImGuiVB[bufferSlot], (voidptr)cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size,
				sizeof(ImDrawVert), 0);
		insigne::copy_update_ib(s_ImGuiIB[bufferSlot], (voidptr)cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size, 0);

		//m_UsedVertexMemory += (cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
		//m_UsedIndexMemory += (cmdList->IdxBuffer.Size * sizeof(s32));

		s32 idxBufferOffset = 0;
		for (s32 cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++)
		{
			const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[cmdIdx];
			s32 x0 = (s32)drawCmd->ClipRect.x;	// topleft
			s32 y0 = fbHeight - (s32)drawCmd->ClipRect.w;
			s32 w = (s32)(drawCmd->ClipRect.z - drawCmd->ClipRect.x);
			s32 h = (s32)(drawCmd->ClipRect.w - drawCmd->ClipRect.y);

			insigne::setup_scissor<ImGuiSurface>(true, x0, y0, w, h);

			static s32 texSlot = insigne::get_material_texture_slot(s_ImGuiMaterial, "u_Tex");
			s_ImGuiMaterial.textures[texSlot].value = *((insigne::texture_handle_t*)drawCmd->TextureId);

			insigne::draw_surface<ImGuiSurface>(s_ImGuiVB[bufferSlot], s_ImGuiIB[bufferSlot], s_ImGuiMaterial,
					idxBufferOffset, (s32)drawCmd->ElemCount);
			idxBufferOffset += drawCmd->ElemCount;
		}
	}
}

void ImGuiCharacterInput(const c8 i_charCode)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharacter(i_charCode);
}

void ImGuiCursorMove(const u32 i_x, const u32 i_y)
{
	s_CursorPos = floral::vec2f((f32)i_x, (f32)i_y);
}

void ImGuiCursorInteract(const bool i_pressed)
{
	s_CursorPressed = i_pressed;
	if (i_pressed)
	{
		s_CursorHeldThisFrame = true;
	}
}

}
