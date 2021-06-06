#pragma once

#include "IDemoHub.h"

#include <floral/containers/array.h>
#include <floral/containers/fast_array.h>
#include <floral/function/simple_callback.h>
#include <floral/io/filesystem.h>

#include "Memory/MemorySystem.h"
#include "InsigneImGui.h"

namespace font_renderer
{
class FontRenderer;
}

namespace stone
{
// ------------------------------------------------------------------

class ITestSuite;

template <class TSuite>
struct SuiteCreator
{
	static ITestSuite* Create()
	{
		return g_PersistanceAllocator.allocate<TSuite>();
	}
};

// ------------------------------------------------------------------

class HWCounter
{
public:
	HWCounter(const s32 i_numSamples, const_cstr i_label);
	~HWCounter();

	f32 PushValue(const f32 i_value);

	void Draw();

private:
	const_cstr									m_Label;
	f32*										m_Values;
	s32											m_CurrentIdx;
	s32											m_PlotIdx;
	s32											m_NumSamples;
	f32											m_MaxValue, m_AvgValue;
};

// ------------------------------------------------------------------

class DemoHub : public IDemoHub
{
private:
	struct SuiteRegistry
	{
		s32										id;
		const_cstr								name;
		floral::simple_callback<ITestSuite*>	createFunction;
	};

public:
	DemoHub(floral::filesystem<FreelistArena>* i_fs);
	~DemoHub();

	void										Initialize();
	void										CleanUp();

	void										OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus);
	void										OnCharacterInput(const c8 i_charCode);
	void										OnCursorMove(const u32 i_x, const u32 i_y);
	void										OnCursorInteract(const bool i_pressed, const u32 i_buttonId);

	void										UpdateFrame(const f32 i_deltaMs);
	void										RenderFrame(const f32 i_deltaMs);

private:
	void										_ShowGPUCounters();
	void										_ShowDebugLogWindow();

private:
	void										_AppLogFunc(const_cstr logStr);
	void										_SwitchTestSuite(SuiteRegistry& i_to);
	void										_ClearAllSuite();

	template <class T>
	void _EmplacePlaygroundSuite()
	{
		SuiteRegistry registry;
		registry.id = m_NextSuiteId;
		registry.name = T::k_name;
		registry.createFunction.bind<&SuiteCreator<T>::Create>();
		m_PlaygroundSuite.push_back(registry);
		m_NextSuiteId++;
	}

	template <class T>
	void _EmplacePerformanceSuite()
	{
		SuiteRegistry registry;
		registry.id = m_NextSuiteId;
		registry.name = T::k_name;
		registry.createFunction.bind<&SuiteCreator<T>::Create>();
		m_PerformanceSuite.push_back(registry);
		m_NextSuiteId++;
	}

	template <class T>
	void _EmplaceRenderTechSuite()
	{
		SuiteRegistry registry;
		registry.id = m_NextSuiteId;
		registry.name = T::k_name;
		registry.createFunction.bind<&SuiteCreator<T>::Create>();
		m_RenderTechSuite.push_back(registry);
		m_NextSuiteId++;
	}

	template <class T>
	void _EmplaceToolSuite()
	{
		SuiteRegistry registry;
		registry.id = m_NextSuiteId;
		registry.name = T::k_name;
		registry.createFunction.bind<&SuiteCreator<T>::Create>();
		m_ToolSuite.push_back(registry);
		m_NextSuiteId++;
	}

	template <class T>
	void _EmplaceMiscSuite()
	{
		SuiteRegistry registry;
		registry.id = m_NextSuiteId;
		registry.name = T::k_name;
		registry.createFunction.bind<&SuiteCreator<T>::Create>();
		m_MiscSuite.push_back(registry);
		m_NextSuiteId++;
	}

	template <class T>
	void _EmplaceImGuiSuite()
	{
		SuiteRegistry registry;
		registry.id = m_NextSuiteId;
		registry.name = T::k_name;
		registry.createFunction.bind<&SuiteCreator<T>::Create>();
		m_ImGuiSuite.push_back(registry);
		m_NextSuiteId++;
	}

private:
	floral::filesystem<FreelistArena>*			m_FileSystem;
    font_renderer::FontRenderer*                m_FontRenderer;
	s32											m_NextSuiteId;
	s32											m_CurrentTestSuiteId;
	ITestSuite*									m_Suite;

private:
	HWCounter									m_GPUCycles;
	HWCounter									m_FragmentCycles;
	HWCounter									m_TilerCycles;
	HWCounter									m_FragElim;
	HWCounter									m_Tiles;
	HWCounter									m_ShaderTextureCycles;
	HWCounter									m_Varying16BitCycles;
	HWCounter									m_Varying32BitCycles;
	HWCounter									m_ExtReadBytes;
	HWCounter									m_ExtWriteBytes;
	HWCounter									m_FrameDurationMs;

	DebugLogWindow<FreelistArena>				m_LogWindow;

private:
	floral::fast_fixed_array<SuiteRegistry, LinearAllocator>	m_PlaygroundSuite;
	floral::fast_fixed_array<SuiteRegistry, LinearAllocator>	m_PerformanceSuite;
	floral::fast_fixed_array<SuiteRegistry, LinearAllocator>	m_RenderTechSuite;
	floral::fast_fixed_array<SuiteRegistry, LinearAllocator>	m_ToolSuite;
	floral::fast_fixed_array<SuiteRegistry, LinearAllocator>	m_MiscSuite;
	floral::fast_fixed_array<SuiteRegistry, LinearAllocator>	m_ImGuiSuite;
};

// ------------------------------------------------------------------
}
