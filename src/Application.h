#pragma once

#include <floral.h>

#include "Memory/MemorySystem.h"

namespace stone {

class Controller;
class ITestSuite;
class IDebugUI;

class Application {
	public:
		Application(Controller* i_controller);
		~Application();

	private:
		void									UpdateFrame(f32 i_deltaMs);
		void									RenderFrame(f32 i_deltaMs);

		void									OnPause();
		void									OnResume();
		void									OnFocusChanged(bool i_hasFocus);
		void									OnDisplayChanged();

		void									OnInitialize();
		void									OnFrameStep(f32 i_deltaMs);
		void									OnCleanUp(int i_param);

		// user interactions
		void									OnCharacterInput(c8 i_character);
		void									OnKeyInput(u32 i_keyCode, u32 i_keyStatus);
		void									OnCursorMove(u32 i_x, u32 i_y);
		void									OnCursorInteract(bool i_pressed, u32 i_buttonId);

	private:
		template <class TTestSuite>
		void _CreateTestSuite()
		{
			TTestSuite* testSuite = g_PersistanceAllocator.allocate<TTestSuite>();
			m_CurrentTestSuite = testSuite;
			m_CurrentTestSuiteUI = testSuite;
		}

	private:
		bool									m_Initialized;
		ITestSuite*								m_CurrentTestSuite;
		IDebugUI*								m_CurrentTestSuiteUI;
};

}
