#include "DemoHub.h"

#include <insigne/ut_render.h>
#include <insigne/counters.h>

#include <clover/Logger.h>

#include "InsigneImGui.h"
#include "Graphics/DebugDrawer.h"

// playground
#include "Graphics/Performance/Vault.h"
#include "Graphics/Misc/GLTFLoader.h"
#include "Graphics/Performance/PostFX.h"

// performance demo
#include "Graphics/Performance/Empty.h"
#include "Graphics/Performance/TextureStreaming.h"
#include "Graphics/Performance/Triangle.h"
#include "Graphics/Performance/ImGuiCustomWidgets.h"
#include "Graphics/Performance/SceneLoader.h"
#include "Graphics/Performance/GammaCorrection.h"
#include "Graphics/Performance/Blending.h"
#include "Graphics/Performance/Textures.h"
#include "Graphics/Performance/CubeMap.h"

// tech demo
#include "Graphics/RenderTech/FrameBuffer.h"
#include "Graphics/RenderTech/PBR.h"
#include "Graphics/RenderTech/PBRHelmet.h"
#include "Graphics/RenderTech/PBRWithIBL.h"
#include "Graphics/RenderTech/FragmentPartition.h"
#include "Graphics/RenderTech/LightProbeGI.h"

// tools demo
#include "Graphics/Tools/SingleAccurateFormFactor.h"
#include "Graphics/Tools/SurfelsGenerator.h"
#include "Graphics/Tools/Samplers.h"
#include "Graphics/Tools/SHCalculator.h"

// misc demo
#include "Graphics/Misc/CameraWork.h"

// imgui demo
#include "Graphics/ImGui/ImGuiDemoWindow.h"
#include "Graphics/ImGui/ValuePlotter.h"

namespace stone
{
//--------------------------------------------------------------------

HWCounter::HWCounter(const s32 i_numSamples, const_cstr i_label)
	: m_Label(i_label)
	, m_CurrentIdx(0)
	, m_PlotIdx(0)
	, m_NumSamples(i_numSamples)
	, m_MaxValue(0.0f)
	, m_AvgValue(0.0f)
{
	m_Values = g_PersistanceAllocator.allocate_array<f32>(i_numSamples);
	memset(m_Values, 0, i_numSamples * sizeof(f32));
}

HWCounter::~HWCounter()
{
	// no need to free, it will be freed after termination either way
	//g_PersistanceAllocator.free(m_Values);
}

void HWCounter::PushValue(const f32 i_value)
{
	m_Values[m_CurrentIdx] = i_value;
	m_PlotIdx = m_CurrentIdx;
	m_CurrentIdx--;
	if (m_CurrentIdx < 0) m_CurrentIdx = m_NumSamples - 1;

	f32 avg = 0.0f;
	m_MaxValue = 0.0f;
	for (s32 i = 0; i < m_NumSamples; i++)
	{
		const f32 v = m_Values[i];
		avg += v;
		if (v > m_MaxValue) m_MaxValue = v;
	}
	avg /= (f32)m_NumSamples;
	m_AvgValue = avg;
}

void HWCounter::Draw()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::PushID(m_Label); ImGui::Text("Avg: %4.2f", m_AvgValue); ImGui::PopID();
	PlotValuesWrap(m_Label, m_Values, 0.0f, m_MaxValue, m_NumSamples, 50 * io.FontGlobalScale,
			m_PlotIdx, IM_COL32(0, 255, 0, 255), IM_COL32(0, 255, 0, 255));
}

//--------------------------------------------------------------------

DemoHub::DemoHub(floral::filesystem<FreelistArena>* i_fs)
	: m_FileSystem(i_fs)
	, m_NextSuiteId(0)
	, m_CurrentTestSuiteId(-1)
	, m_Suite(nullptr)

	, m_GPUCycles(64, "GPU Cycles")
	, m_FragmentCycles(64, "Fragment Cycles")
	, m_TilerCycles(64, "Tiler Cycles")
	, m_ExtReadBytes(64, "Ext Read Bytes")
	, m_ExtWriteBytes(64, "Ext Write Bytes")
{
}

DemoHub::~DemoHub()
{
}

void DemoHub::Initialize()
{
	m_PlaygroundSuite.reserve(4, &g_PersistanceAllocator);
	m_PerformanceSuite.reserve(16, &g_PersistanceAllocator);
	m_RenderTechSuite.reserve(16, &g_PersistanceAllocator);
	m_ToolSuite.reserve(16, &g_PersistanceAllocator);
	m_MiscSuite.reserve(16, &g_PersistanceAllocator);
	m_ImGuiSuite.reserve(8, &g_PersistanceAllocator);

	InitializeImGui();
	debugdraw::Initialize();

	_EmplacePlaygroundSuite<perf::Vault>();
	//_EmplacePlaygroundSuite<perf::PostFX>();

	_EmplacePerformanceSuite<perf::Empty>();
	_EmplacePerformanceSuite<perf::Triangle>();
	_EmplacePerformanceSuite<perf::SceneLoader>();
	//_EmplacePerformanceSuite<perf::GammaCorrection>();
	_EmplacePerformanceSuite<perf::LDRBlending>();
	_EmplacePerformanceSuite<perf::HDRBlending>();
	_EmplacePerformanceSuite<perf::Textures>();
	_EmplacePerformanceSuite<perf::CubeMapTextures>();

	_EmplaceRenderTechSuite<tech::PBRHelmet>();

#if defined(FLORAL_PLATFORM_WINDOWS)
	_EmplaceToolSuite<tools::SHCalculator>();
#endif

	//_EmplaceMiscSuite<misc::CameraWork>();

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
			if (m_PlaygroundSuite.get_size() > 0)
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
			}

			if (m_PerformanceSuite.get_size() > 0)
			{
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
			}

			if (m_RenderTechSuite.get_size() > 0)
			{
				if (ImGui::BeginMenu("Render Tech"))
				{
					for (ssize i = 0; i < m_RenderTechSuite.get_size(); i++)
					{
						SuiteRegistry& suiteRegistry = m_RenderTechSuite[i];
						if (ImGui::MenuItem(suiteRegistry.name, nullptr))
						{
							_SwitchTestSuite(suiteRegistry);
						}
					}
					ImGui::EndMenu();
				}
			}

			if (m_ToolSuite.get_size() > 0)
			{
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
			}

			if (m_MiscSuite.get_size() > 0)
			{
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
			}

			if (m_ImGuiSuite.get_size() > 0)
			{
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
			}

			if (ImGui::MenuItem("Clear all suites"))
			{
				_ClearAllSuite();
			}
			ImGui::EndMenu();
		}

		static bool s_showGPUCounters = false;
		if (ImGui::BeginMenu("Profiler"))
		{
			ImGui::MenuItem("GPU Counters", nullptr, &s_showGPUCounters);
			ImGui::EndMenu();
		}

		if (m_Suite)
		{
			ImGui::MenuItem(m_Suite->GetName(), nullptr, nullptr, false);
		}
		ImGui::EndMainMenuBar();

		if (s_showGPUCounters)
		{
			_ShowGPUCounters();
		}
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

//--------------------------------------------------------------------

void DemoHub::_ShowGPUCounters()
{
	const u64 validIdx = insigne::get_valid_debug_info_range_end();
	f32 gpuCycles = insigne::g_hardware_counters->gpu_cycles[validIdx];
	f32 fragmentCycles = insigne::g_hardware_counters->fragment_cycles[validIdx];
	f32 tilerCycles = insigne::g_hardware_counters->tiler_cycles[validIdx];
	f32 extReadBytes = insigne::g_hardware_counters->external_memory_read_bytes[validIdx];
	f32 extWriteBytes = insigne::g_hardware_counters->external_memory_write_bytes[validIdx];

	m_GPUCycles.PushValue(gpuCycles);
	m_FragmentCycles.PushValue(fragmentCycles);
	m_TilerCycles.PushValue(tilerCycles);
	m_ExtReadBytes.PushValue(extReadBytes);
	m_ExtWriteBytes.PushValue(extWriteBytes);

	ImGui::Begin("GPU Counters");
	m_GPUCycles.Draw();
	m_FragmentCycles.Draw();
	m_TilerCycles.Draw();
	m_ExtReadBytes.Draw();
	m_ExtWriteBytes.Draw();
	ImGui::End();
}

//--------------------------------------------------------------------

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
		m_Suite->OnInitialize(m_FileSystem);
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

//--------------------------------------------------------------------
}
