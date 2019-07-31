#include "DemoHub.h"

#include <insigne/ut_render.h>

#include <imgui.h>

#include "InsigneImGui.h"

namespace stone
{

DemoHub::DemoHub()
	: m_CurrentTestSuite(nullptr)
{
}

DemoHub::~DemoHub()
{
}

void DemoHub::Initialize()
{
	InitializeImGui();
}

void DemoHub::CleanUp()
{
}

void DemoHub::OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus)
{
}

void DemoHub::OnCharacterInput(const c8 i_charCode)
{
	ImGuiCharacterInput(i_charCode);
}

void DemoHub::OnCursorMove(const u32 i_x, const u32 i_y)
{
	ImGuiCursorMove(i_x, i_y);
}

void DemoHub::OnCursorInteract(const bool i_pressed, const u32 i_buttonId)
{
	ImGuiCursorInteract(i_pressed);
}

void DemoHub::UpdateFrame(const f32 i_deltaMs)
{
	UpdateImGui();
	ImGui::NewFrame();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("TestSuite"))
		{
			ImGui::MenuItem("<empty suite>", nullptr);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	//ImGui::ShowTestWindow();
}

void DemoHub::RenderFrame(const f32 i_deltaMs)
{
	if (m_CurrentTestSuite)
	{
	}
	else
	{
		insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

		RenderImGui();

		insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
		insigne::mark_present_render();
		insigne::dispatch_render_pass();
	}
}

}
