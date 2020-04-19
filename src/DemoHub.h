#pragma once

#include "IDemoHub.h"

#include <floral/containers/array.h>
#include <floral/containers/fast_array.h>
#include <floral/function/simple_callback.h>

#include "Memory/MemorySystem.h"

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
	DemoHub();
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
	s32											m_NextSuiteId;
	s32											m_CurrentTestSuiteId;
	ITestSuite*									m_Suite;

private:
	floral::fast_fixed_array<SuiteRegistry, LinearAllocator>	m_PlaygroundSuite;
	floral::fast_fixed_array<SuiteRegistry, LinearAllocator>	m_PerformanceSuite;
	floral::fast_fixed_array<SuiteRegistry, LinearAllocator>	m_ToolSuite;
	floral::fast_fixed_array<SuiteRegistry, LinearAllocator>	m_ImGuiSuite;
};

// ------------------------------------------------------------------
}
