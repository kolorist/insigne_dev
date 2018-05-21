#include "Debugger.h"

namespace stone {

	Debugger::Debugger(MaterialManager* i_materialManager)
		: m_MouseX(0.0f)
		, m_MouseY(0.0f)
		, m_MaterialManager(i_materialManager)
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

		// fonts
		u8* pixels;
		s32 width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		m_UIMaterial = m_MaterialManager->CreateMaterial<DebugUIMaterial>("shaders/internal/debugui");
	}

	void Debugger::Update(f32 i_deltaMs)
	{
		ImGuiIO& io = ImGui::GetIO();

		io.MousePos = ImVec2(m_MouseX, m_MouseY);
		for (u32 i = 0; i < 2; i++) {
			io.MouseDown[i] = m_MouseHeldThisFrame[i] || m_MousePressed[i];
			m_MouseHeldThisFrame[i] = false;
		}
		io.DeltaTime = i_deltaMs;
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
