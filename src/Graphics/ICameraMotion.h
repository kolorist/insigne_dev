#pragma once

#include <floral.h>

namespace stone {

class ICameraMotion {
	public:
		virtual void							OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus) = 0;
		virtual void							OnCursorMove(const u32 i_x, const u32 i_y) = 0;
		virtual void							OnCursorInteract(const bool i_pressed) = 0;

		virtual void							OnUpdate(const f32 i_deltaMs) = 0;
};

}
