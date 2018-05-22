#include "Debugger.h"

#include <context.h>

#include "DebugUIMaterial.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

	static insigne::surface_handle_t			s_UISurface;
	static DebugUIMaterial*						s_UIMaterial;

	Debugger::Debugger(MaterialManager* i_materialManager, ITextureManager* i_textureManager)
		: m_MouseX(0.0f)
		, m_MouseY(0.0f)
		, m_MaterialManager(i_materialManager)
		, m_TextureManager(i_textureManager)
	{
		for (u32 i = 0; i < 2; i++) {
			m_MousePressed[i] = false;
			m_MouseHeldThisFrame[i] = false;
		}
	}

	Debugger::~Debugger()
	{
	}

	void Debugger::Initialize()
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
	}

	void Debugger::Update(f32 i_deltaMs)
	{
		ImGuiIO& io = ImGui::GetIO();

		io.MousePos = ImVec2(m_MouseX, m_MouseY);
		for (u32 i = 0; i < 2; i++) {
			io.MouseDown[i] = m_MouseHeldThisFrame[i] || m_MousePressed[i];
			m_MouseHeldThisFrame[i] = false;
		}
		io.DeltaTime = i_deltaMs / 1000.0f;
		ImGui::NewFrame();
		ImGui::ShowTestWindow();
	}

	void Debugger::Render(f32 i_deltaMs)
	{
		ImGui::Render();
	}
	// -----------------------------------------
	void Debugger::RenderImGuiDrawLists(ImDrawData* i_drawData)
	{
		ImGuiIO& io = ImGui::GetIO();
		i_drawData->ScaleClipRects(ImVec2(1.0f, 1.0f));
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
					insigne::draw_surface_segmented<ImGuiSurface>(s_UISurface,
						s_UIMaterial->GetHandle(),
						(s32)drawCmd->ElemCount, (voidptr)idxBufferOffset);
				}
				idxBufferOffset += drawCmd->ElemCount;
			}
		}
	}

	// -----------------------------------------
	void Debugger::OnCharacterInput(c8 i_character)
	{
	}

	void Debugger::OnCursorMove(u32 i_x, u32 i_y)
	{
		m_MouseX = (f32)i_x;
		m_MouseY = (f32)i_y;
	}

	void Debugger::OnCursorInteract(bool i_pressed, u32 i_buttonId)
	{
		// 1 = left mouse (touch)
		// 2 = right mouse
		m_MousePressed[i_buttonId - 1] = i_pressed;
		if (i_pressed)
			m_MouseHeldThisFrame[i_buttonId - 1] = true;
	}

}
