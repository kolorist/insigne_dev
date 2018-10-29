#pragma once

#include <floral.h>

#include "ICameraMotion.h"

namespace stone {

class TrackballCamera : public ICameraMotion {
	public:
		TrackballCamera();

		void									OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus) override;
		void									OnCursorMove(const u32 i_x, const u32 i_y) override;
		void									OnCursorInteract(const bool i_pressed) override;

		floral::quaternionf						GetRotation() { return m_Rotation * m_LastRotation; }
		floral::vec3f							GetStartCoord() { return m_StartCoord; }
		floral::vec3f							GetEndCoord() { return m_EndCoord; }

	private:
		bool									m_CursorPressed;
		floral::vec3f							m_SphereCoord;
		floral::vec3f							m_StartCoord, m_EndCoord;
		floral::quaternionf						m_Rotation;
		floral::quaternionf						m_LastRotation;
		floral::mat4x4f							m_InverseView;
};

}
