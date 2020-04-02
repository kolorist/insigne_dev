#pragma once

#include <floral.h>

#include "ICameraMotion.h"

namespace stone {

class TrackballCamera : public ICameraMotion
{
public:
	TrackballCamera(const floral::camera_view_t& i_view, const floral::camera_persp_t& i_proj);

	void										SetScreenResolution(const u32 i_width, const u32 i_height);

	void										OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus) override;
	void										OnCursorMove(const u32 i_x, const u32 i_y) override;
	void										OnCursorInteract(const bool i_pressed) override;

	void										OnUpdate(const f32 i_deltaMs) override;

	floral::mat4x4f								GetWVP() const;
	floral::vec3f								GetPosition() const;

public:
	const bool									IsCursorPressed() const { return m_CursorPressed; }
	const floral::vec2i&						GetPrevCursorPos() const { return m_PrevCursorPos; }
	const floral::vec2i&						GetCursorPos() const { return m_CursorPos; }
	floral::vec3f								GetArcCursor() const { return _GetArcCursor(m_CursorPos); }
	floral::vec3f								GetArcPrevCursor() const { return _GetArcCursor(m_PrevCursorPos); }
	floral::quaternionf							GetRotation() const { return (m_Rotation * m_PrevRotation).normalize(); }

private:
	floral::vec3f								_GetArcCursor(const floral::vec2i& i_vec) const;

private:
	bool										m_CursorPressed;
	floral::vec2i								m_PrevCursorPos;
	floral::vec2i								m_CursorPos;
	f32											m_Radius;
	s32											m_HalfWidth;
	s32											m_HalfHeight;

	floral::quaternionf							m_PrevRotation;
	floral::quaternionf							m_Rotation;

	floral::mat4x4f								m_InverseView;
	floral::mat4x4f								m_Projection;
	floral::camera_view_t						m_CamView;
};

}
