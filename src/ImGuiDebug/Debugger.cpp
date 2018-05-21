#include "Debugger.h"

namespace stone {

	Debugger::Debugger()
		: m_MouseX(0.0f)
		, m_MouseY(0.0f)
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
	}

	void Debugger::Update(f32 i_deltaMs)
	{
	}

	void Debugger::Render(f32 i_deltaMs)
	{
		ImGui::NewFrame();
		ImGui::ShowTestWindow();
		ImGui::Render();
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
	}

}
