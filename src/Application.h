#pragma once

#include <floral/stdaliases.h>
#include <floral/io/filesystem.h>

#include "Memory/MemorySystem.h"

namespace stone
{
// -------------------------------------------------------------------

class Controller;
class ITestSuite;
class IDebugUI;
class IDemoHub;

class Application
{
public:
	Application(Controller* i_controller);
	~Application();

private:
	void										UpdateFrame(f32 i_deltaMs);
	void										RenderFrame(f32 i_deltaMs);

	void										OnPause();
	void										OnResume();
	void										OnFocusChanged(bool i_hasFocus);
	void										OnDisplayChanged();

	void										OnInitializePlatform();
	void										OnInitializeRenderer();
	void										OnInitializeGame();
	void										OnFrameStep(f32 i_deltaMs);
	void										OnCleanUp();

	// user interactions
	void										OnCharacterInput(c8 i_character);
	void										OnKeyInput(u32 i_keyCode, u32 i_keyStatus);
	void										OnCursorMove(u32 i_x, u32 i_y);
	void										OnCursorInteract(bool i_pressed, u32 i_buttonId);

private:
	bool										m_Initialized;
	floral::filesystem<FreelistArena>*			m_FileSystem;
	IDemoHub*									m_DemoHub;

private:
	FreelistArena*								m_FSMemoryArena;
};

// -------------------------------------------------------------------
}
