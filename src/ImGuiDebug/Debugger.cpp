#include "Debugger.h"

#include <context.h>

#include <insigne/counters.h>

#include "DebugUIMaterial.h"
#include "Graphics/Camera.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/ITextureManager.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

static insigne::surface_handle_t				s_UISurface;
static DebugUIMaterial*							s_UIMaterial;

floral::fixed_array<lotus::unpacked_event, LinearAllocator>	s_profileEvents[4];

Debugger::Debugger(MaterialManager* i_materialManager, ITextureManager* i_textureManager)
	: m_MouseX(0.0f)
	, m_MouseY(0.0f)
	, m_MaterialManager(i_materialManager)
	, m_TextureManager(i_textureManager)
	, m_Camera(nullptr)
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
	//ImGui::ShowTestWindow();
	{
		c8 windowName[50];
		ImGui::Begin("Profiler");
		ImGui::LabelText("Submit Frame Idx", "%d", insigne::g_global_counters.current_submit_frame_idx);
		ImGui::LabelText("Render Frame Idx", "%d", insigne::g_global_counters.current_render_frame_idx);
		if (ImGui::Button("Load Default Textures")) {
			OnRequestLoadDefaultTextures();
		}

		if (ImGui::Button("Load Plate Material")) {
			OnRequestLoadPlateMaterial();
		}

		if (ImGui::Button("Construct Camera")) {
			OnRequestConstructCamera();
		}

		if (ImGui::Button("Load Models")) {
			OnRequestLoadModels();
		}

		if (ImGui::Button("Load and Apply Textures")) {
			OnRequestLoadAndApplyTextures();
		}

		if (ImGui::Button("Load Skybox")) {
			OnRequestLoadSkybox();
		}

		if (ImGui::Button("Load Shading Probes")) {
			OnRequestLoadShadingProbes();
		}

		if (ImGui::Button("Load Split Sum LUT")) {
			OnRequestLoadLUTTexture();
		}

		if (m_Camera) {
			if (ImGui::CollapsingHeader("Camera Debug")) {
				ImGui::LabelText("position", "(%f; %f; %f)", m_Camera->Position.x, m_Camera->Position.y, m_Camera->Position.z);
				ImGui::LabelText("look_at_direction", "(%f; %f; %f)", m_Camera->LookAtDir.x, m_Camera->LookAtDir.y, m_Camera->LookAtDir.z);
				ImGui::LabelText("aspect_ratio", "%f", m_Camera->AspectRatio);
				ImGui::LabelText("plane", "%f - %f", m_Camera->NearPlane, m_Camera->FarPlane);
				ImGui::LabelText("field_of_view", "%f", m_Camera->FOV);
				ImGui::Text("view_matrix:");
				ImGui::Text("%4.2f; %4.2f; %4.2f; %4.2f",
						m_Camera->ViewMatrix[0][0], m_Camera->ViewMatrix[0][1], m_Camera->ViewMatrix[0][2], m_Camera->ViewMatrix[0][3]);
				ImGui::Text("%4.2f; %4.2f; %4.2f; %4.2f",
						m_Camera->ViewMatrix[1][0], m_Camera->ViewMatrix[1][1], m_Camera->ViewMatrix[1][2], m_Camera->ViewMatrix[1][3]);
				ImGui::Text("%4.2f; %4.2f; %4.2f; %4.2f",
						m_Camera->ViewMatrix[2][0], m_Camera->ViewMatrix[2][1], m_Camera->ViewMatrix[2][2], m_Camera->ViewMatrix[2][3]);
				ImGui::Text("%4.2f; %4.2f; %4.2f; %4.2f",
						m_Camera->ViewMatrix[3][0], m_Camera->ViewMatrix[3][1], m_Camera->ViewMatrix[3][2], m_Camera->ViewMatrix[3][3]);
				ImGui::Text("projection_matrix:");
				ImGui::Text("%4.2f; %4.2f; %4.2f; %4.2f",
						m_Camera->ProjectionMatrix[0][0], m_Camera->ProjectionMatrix[0][1], m_Camera->ProjectionMatrix[0][2], m_Camera->ProjectionMatrix[0][3]);
				ImGui::Text("%4.2f; %4.2f; %4.2f; %4.2f",
						m_Camera->ProjectionMatrix[1][0], m_Camera->ProjectionMatrix[1][1], m_Camera->ProjectionMatrix[1][2], m_Camera->ProjectionMatrix[1][3]);
				ImGui::Text("%4.2f; %4.2f; %4.2f; %4.2f",
						m_Camera->ProjectionMatrix[2][0], m_Camera->ProjectionMatrix[2][1], m_Camera->ProjectionMatrix[2][2], m_Camera->ProjectionMatrix[2][3]);
				ImGui::Text("%4.2f; %4.2f; %4.2f; %4.2f",
						m_Camera->ProjectionMatrix[3][0], m_Camera->ProjectionMatrix[3][1], m_Camera->ProjectionMatrix[3][2], m_Camera->ProjectionMatrix[3][3]);
				ImGui::Text("wvp_matrix:");
				ImGui::Text("%4.2f; %4.2f; %4.2f; %4.2f",
						m_Camera->WVPMatrix[0][0], m_Camera->WVPMatrix[0][1], m_Camera->WVPMatrix[0][2], m_Camera->WVPMatrix[0][3]);
				ImGui::Text("%4.2f; %4.2f; %4.2f; %4.2f",
						m_Camera->WVPMatrix[1][0], m_Camera->WVPMatrix[1][1], m_Camera->WVPMatrix[1][2], m_Camera->WVPMatrix[1][3]);
				ImGui::Text("%4.2f; %4.2f; %4.2f; %4.2f",
						m_Camera->WVPMatrix[2][0], m_Camera->WVPMatrix[2][1], m_Camera->WVPMatrix[2][2], m_Camera->WVPMatrix[2][3]);
				ImGui::Text("%4.2f; %4.2f; %4.2f; %4.2f",
						m_Camera->WVPMatrix[3][0], m_Camera->WVPMatrix[3][1], m_Camera->WVPMatrix[3][2], m_Camera->WVPMatrix[3][3]);
			}
		}

		for (sidx n = 0; n < 2; n++) {
			sprintf(windowName, "Capture #%lld", n);
			if (ImGui::CollapsingHeader(windowName)) {
				for (u32 i = 0; i < s_profileEvents[n].get_size(); i++) {
					lotus::unpacked_event& thisEvent = s_profileEvents[n][i];
					ImGui::Indent(thisEvent.depth * 10.0f);
					ImGui::Text("%-35s - %f", thisEvent.name, thisEvent.duration_ms);
					ImGui::Unindent(thisEvent.depth * 10.0f);
				}
			}
		}
		ImGui::End();
	}
}

void Debugger::Render(f32 i_deltaMs)
{
	ImGui::Render();
}

// -----------------------------------------
void Debugger::RenderImGuiDrawLists(ImDrawData* i_drawData)
{
	ImGuiIO& io = ImGui::GetIO();
	s32 fbWidth = (s32)(io.DisplaySize.x * 1.0f); //io.DisplayFramebufferScale.x);
	s32 fbHeight = (s32)(io.DisplaySize.y * 1.0f); //io.DisplayFramebufferScale.y);

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
				s32 x0 = drawCmd->ClipRect.x;	// topleft
				s32 y0 = drawCmd->ClipRect.y;
				s32 w = drawCmd->ClipRect.z - drawCmd->ClipRect.x;
				s32 h = drawCmd->ClipRect.w - drawCmd->ClipRect.y;

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
