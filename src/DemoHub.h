#pragma once

#include "IDemoHub.h"

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
	ITestSuite*									m_CurrentTestSuite;
};

}
