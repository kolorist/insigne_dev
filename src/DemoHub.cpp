#include "DemoHub.h"

#include <insigne/ut_render.h>

#include <clover/Logger.h>

#include "InsigneImGui.h"
#include "Graphics/DebugDrawer.h"

// playground
#include "Graphics/Performance/Vault.h"
#include "Graphics/Misc/GLTFLoader.h"

// performance demo
#include "Graphics/Performance/Empty.h"
#include "Graphics/Performance/TextureStreaming.h"
#include "Graphics/Performance/Triangle.h"
#include "Graphics/Performance/ImGuiCustomWidgets.h"
#include "Graphics/Performance/SceneLoader.h"
#include "Graphics/Performance/GammaCorrection.h"

// tech demo
#include "Graphics/RenderTech/FrameBuffer.h"
#include "Graphics/RenderTech/PBR.h"
#include "Graphics/RenderTech/PBRWithIBL.h"
#include "Graphics/RenderTech/SHCalculator.h"
#include "Graphics/RenderTech/FragmentPartition.h"
#include "Graphics/RenderTech/LightProbeGI.h"

// tools demo
#include "Graphics/Tools/SingleAccurateFormFactor.h"
#include "Graphics/Tools/SurfelsGenerator.h"
#include "Graphics/Tools/Samplers.h"

// misc demo
#include "Graphics/Misc/CameraWork.h"

// imgui demo
#include "Graphics/ImGui/ImGuiDemoWindow.h"
#include "Graphics/ImGui/ValuePlotter.h"

namespace stone
{

DemoHub::DemoHub()
	: m_NextSuiteId(0)
	, m_CurrentTestSuiteId(-1)
	, m_Suite(nullptr)
{
}

DemoHub::~DemoHub()
{
}

void DemoHub::Initialize()
{
	m_PlaygroundSuite.reserve(4, &g_PersistanceAllocator);
	m_PerformanceSuite.reserve(16, &g_PersistanceAllocator);
	m_ToolSuite.reserve(16, &g_PersistanceAllocator);
	m_MiscSuite.reserve(16, &g_PersistanceAllocator);
	m_ImGuiSuite.reserve(8, &g_PersistanceAllocator);

	InitializeImGui();
	debugdraw::Initialize();

	_EmplacePlaygroundSuite<perf::Vault>();
	_EmplacePlaygroundSuite<misc::GLTFLoader>();

	_EmplacePerformanceSuite<perf::Empty>();
	_EmplacePerformanceSuite<perf::Triangle>();
	_EmplacePerformanceSuite<perf::SceneLoader>();
	_EmplacePerformanceSuite<perf::GammaCorrection>();

	_EmplaceToolSuite<tech::SHCalculator>();

	_EmplaceMiscSuite<misc::CameraWork>();

	_EmplaceImGuiSuite<gui::ImGuiDemoWindow>();
	_EmplaceImGuiSuite<gui::ValuePlotter>();
}

void DemoHub::CleanUp()
{
	_ClearAllSuite();
	debugdraw::CleanUp();
}

void DemoHub::OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus)
{
	bool consumed = false;
	if (i_keyStatus == 0 || i_keyStatus == 1) // pressed or held
	{
		consumed |= ImGuiKeyInput(i_keyCode, true);
	}
	else if (i_keyStatus == 2) // up
	{
		consumed |= ImGuiKeyInput(i_keyCode, false);
	}

	if (!consumed)
	{
		if (m_Suite)
		{
			if (m_Suite->GetCameraMotion())
			{
				m_Suite->GetCameraMotion()->OnKeyInput(i_keyCode, i_keyStatus);
			}
		}
	}
}

void DemoHub::OnCharacterInput(const c8 i_charCode)
{
	ImGuiCharacterInput(i_charCode);
}

void DemoHub::OnCursorMove(const u32 i_x, const u32 i_y)
{
	const bool consumed = ImGuiCursorMove(i_x, i_y);
	if (!consumed)
	{
		if (m_Suite)
		{
			if (m_Suite->GetCameraMotion())
			{
				m_Suite->GetCameraMotion()->OnCursorMove(i_x, i_y);
			}
		}
	}
}

void DemoHub::OnCursorInteract(const bool i_pressed, const u32 i_buttonId)
{
	const bool consumed = ImGuiCursorInteract(i_pressed);
	if (!consumed)
	{
		if (m_Suite)
		{
			if (m_Suite->GetCameraMotion())
			{
				m_Suite->GetCameraMotion()->OnCursorInteract(i_pressed);
			}
		}
	}
}

void DemoHub::UpdateFrame(const f32 i_deltaMs)
{
	UpdateImGui();
	ImGui::NewFrame();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("TestSuite"))
		{
			if (ImGui::BeginMenu("Playground"))
			{
				for (ssize i = 0; i < m_PlaygroundSuite.get_size(); i++)
				{
					SuiteRegistry& suiteRegistry = m_PlaygroundSuite[i];
					if (ImGui::MenuItem(suiteRegistry.name, nullptr))
					{
						_SwitchTestSuite(suiteRegistry);
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Performance"))
			{
				for (ssize i = 0; i < m_PerformanceSuite.get_size(); i++)
				{
					SuiteRegistry& suiteRegistry = m_PerformanceSuite[i];
					if (ImGui::MenuItem(suiteRegistry.name, nullptr))
					{
						_SwitchTestSuite(suiteRegistry);
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Render tech"))
			{
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Tools"))
			{
				for (ssize i = 0; i < m_ToolSuite.get_size(); i++)
				{
					SuiteRegistry& suiteRegistry = m_ToolSuite[i];
					if (ImGui::MenuItem(suiteRegistry.name, nullptr))
					{
						_SwitchTestSuite(suiteRegistry);
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Misc"))
			{
				for (ssize i = 0; i < m_MiscSuite.get_size(); i++)
				{
					SuiteRegistry& suiteRegistry = m_MiscSuite[i];
					if (ImGui::MenuItem(suiteRegistry.name, nullptr))
					{
						_SwitchTestSuite(suiteRegistry);
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("ImGui"))
			{
				for (ssize i = 0; i < m_ImGuiSuite.get_size(); i++)
				{
					SuiteRegistry& suiteRegistry = m_ImGuiSuite[i];
					if (ImGui::MenuItem(suiteRegistry.name, nullptr))
					{
						_SwitchTestSuite(suiteRegistry);
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Clear all suites"))
			{
				_ClearAllSuite();
			}
			ImGui::EndMenu();
		}

		if (m_Suite)
		{
			ImGui::MenuItem(m_Suite->GetName(), nullptr, nullptr, false);
		}
		ImGui::EndMainMenuBar();
	}

	debugdraw::BeginFrame();
	if (m_Suite)
	{
		m_Suite->OnUpdate(i_deltaMs);
	}
	debugdraw::EndFrame();
}

void DemoHub::RenderFrame(const f32 i_deltaMs)
{
	if (m_Suite)
	{
		m_Suite->OnRender(i_deltaMs);
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

void DemoHub::_SwitchTestSuite(SuiteRegistry& i_to)
{
	if (i_to.id != m_CurrentTestSuiteId)
	{
		if (m_Suite)
		{
			m_Suite->OnCleanUp();
			g_PersistanceAllocator.free(m_Suite);
			m_Suite = nullptr;
		}

		m_CurrentTestSuiteId = i_to.id;
		m_Suite = i_to.createFunction();
		m_Suite->OnInitialize();
	}
}

void DemoHub::_ClearAllSuite()
{
	m_CurrentTestSuiteId = -1;
	if (m_Suite)
	{
		m_Suite->OnCleanUp();
		g_PersistanceAllocator.free(m_Suite);
		m_Suite = nullptr;
	}
}

}
