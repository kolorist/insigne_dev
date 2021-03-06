#pragma once

#include <floral.h>

#include "ICameraMotion.h"

namespace stone
{

class FreeCamera : public ICameraMotion
{
public:
	FreeCamera(const floral::camera_view_t& i_view, const floral::camera_persp_t& i_proj);

	void										OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus) override;
	void										OnCursorMove(const u32 i_x, const u32 i_y) override;
	void										OnCursorInteract(const bool i_pressed) override;

	void										OnUpdate(const f32 i_deltaMs) override;

	void										SetProjection(const f32 i_near, const f32 i_far, const f32 i_fov, const f32 i_ratio);

	floral::mat4x4f								GetWVP() { return m_Projection * m_View; }
	const floral::vec2f&						GetCursorPos() { return m_CursorPos; }
	const floral::vec3f&						GetPosition() { return m_CamView.position; }

private:
	void										_UpdateMatrices();

private:
	floral::vec2f								m_CursorPos;
	floral::vec3f								m_Orbital;

	floral::vec3f								m_LeftDir;
	floral::vec3f								m_Forward;

	floral::camera_view_t						m_CamView;
	floral::camera_persp_t						m_CamProj;

	u32											m_KeyStates;
	floral::mat4x4f								m_Projection;
	floral::mat4x4f								m_View;
};

}
