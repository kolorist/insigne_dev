#include "DemoHub.h"

#include <insigne/ut_render.h>

#include <imgui.h>

#include "InsigneImGui.h"
#include "Graphics/DebugDrawer.h"

// performance demo
#include "Graphics/Performance/Empty.h"
#include "Graphics/Performance/Triangle.h"

// tech demo
#include "Graphics/RenderTech/FrameBuffer.h"

// tools demo
#include "Graphics/Tools/SingleAccurateFormFactor.h"

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
	debugdraw::Initialize();

	_EmplacePerformanceSuite<perf::Empty>();
	_EmplacePerformanceSuite<perf::Triangle>();

	_EmplaceRenderTechSuite<tech::FrameBuffer>();

	_EmplaceToolSuite<tools::SingleAccurateFormFactor>();
}

void DemoHub::CleanUp()
{
	debugdraw::CleanUp();
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
			if (ImGui::BeginMenu("Performance"))
			{
				for (ssize i = 0; i < m_PerformanceSuite.get_size(); i++)
				{
					ITestSuite* suite = m_PerformanceSuite[i];
					if (ImGui::MenuItem(suite->GetName(), nullptr))
					{
						_SwitchTestSuite(suite);
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Render tech"))
			{
				for (ssize i = 0; i < m_RenderTechSuite.get_size(); i++)
				{
					ITestSuite* suite = m_RenderTechSuite[i];
					if (ImGui::MenuItem(suite->GetName(), nullptr))
					{
						_SwitchTestSuite(suite);
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Tools"))
			{
				for (ssize i = 0; i < m_ToolSuite.get_size(); i++)
				{
					ITestSuite* suite = m_ToolSuite[i];
					if (ImGui::MenuItem(suite->GetName(), nullptr))
					{
						_SwitchTestSuite(suite);
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Clear all suites"))
			{
				_SwitchTestSuite(nullptr);
			}
			ImGui::EndMenu();
		}

		if (m_CurrentTestSuite)
		{
			ImGui::MenuItem(m_CurrentTestSuite->GetName(), nullptr, nullptr, false);
		}
		ImGui::EndMainMenuBar();
	}

	debugdraw::BeginFrame();
	if (m_CurrentTestSuite)
	{
		m_CurrentTestSuite->OnUpdate(i_deltaMs);
	}
	debugdraw::EndFrame();

	//ImGui::ShowTestWindow();
}

void DemoHub::RenderFrame(const f32 i_deltaMs)
{
	if (m_CurrentTestSuite)
	{
		m_CurrentTestSuite->OnRender(i_deltaMs);
	}
	else
	{
		insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

		debugdraw::Render(floral::mat4x4f(1.0f));
		RenderImGui();

		insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
		insigne::mark_present_render();
		insigne::dispatch_render_pass();
	}
}

//----------------------------------------------

void DemoHub::_SwitchTestSuite(ITestSuite* i_to)
{
	if (i_to != m_CurrentTestSuite)
	{
		if (m_CurrentTestSuite)
		{
			m_CurrentTestSuite->OnCleanUp();
		}

		m_CurrentTestSuite = i_to;
		if (m_CurrentTestSuite)
		{
			m_CurrentTestSuite->OnInitialize();
		}
	}
}

}
