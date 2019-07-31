#pragma once

#include <floral/stdaliases.h>

namespace stone
{

class IDemoHub
{
public:
	virtual ~IDemoHub() = default;

	virtual void								Initialize() = 0;
	virtual void								CleanUp() = 0;

	virtual void								OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus) = 0;
	virtual void								OnCharacterInput(const c8 i_charCode) = 0;
	virtual void								OnCursorMove(const u32 i_x, const u32 i_y) = 0;
	virtual void								OnCursorInteract(const bool i_pressed, const u32 i_buttonId) = 0;

	virtual void								UpdateFrame(const f32 i_deltaMs) = 0;
	virtual void								RenderFrame(const f32 i_deltaMs) = 0;
};

}
