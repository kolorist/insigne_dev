#pragma once

#include "IDemoHub.h"

#include <floral/containers/array.h>

namespace stone
{

class ITestSuite;

class DemoHub : public IDemoHub
{
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
	void										_SwitchTestSuite(ITestSuite* i_to);
	template <class T>
	void _EmplacePerformanceSuite()
	{
		ITestSuite* newSuite = g_PersistanceAllocator.allocate<T>();
		m_PerformanceSuite.push_back(newSuite);
	}

	template <class T>
	void _EmplaceRenderTechSuite()
	{
		ITestSuite* newSuite = g_PersistanceAllocator.allocate<T>();
		m_RenderTechSuite.push_back(newSuite);
	}

	template <class T>
	void _EmplaceToolSuite()
	{
		ITestSuite* newSuite = g_PersistanceAllocator.allocate<T>();
		m_ToolSuite.push_back(newSuite);
	}

private:
	floral::inplace_array<ITestSuite*, 8>		m_PerformanceSuite;
	floral::inplace_array<ITestSuite*, 8>		m_RenderTechSuite;
	floral::inplace_array<ITestSuite*, 8>		m_ToolSuite;

	ITestSuite*									m_CurrentTestSuite;
};

}