#pragma once

#include <floral.h>

#include "ICameraMotion.h"

namespace stone
{

class FreeCamera : public ICameraMotion
{
	public:
		FreeCamera(const floral::vec3f& i_position, const floral::vec3f i_upDir, const floral::vec3f i_fwDir);

		void									OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus) override;
		void									OnCursorMove(const u32 i_x, const u32 i_y) override;
		void									OnCursorInteract(const bool i_pressed) override;

		void									OnUpdate(const f32 i_deltaMs) override;

	private:
		floral::vec3f							m_Position;
		floral::vec3f							m_UpDir;
		floral::vec3f							m_LeftDir;
		floral::vec3f							m_Forward;

		u32										m_KeyStates;
};

}
