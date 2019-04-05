#include "FreeCamera.h"

#include <clover.h>
#include <calyx/platform/windows/event_defs.h>

namespace stone
{

static const f32 k_CameraSpeed = 5.0f;
static const f32 k_SpeedMultiplier = 0.01f;

FreeCamera::FreeCamera(const floral::camera_view_t& i_view, const floral::camera_persp_t& i_proj)
	: m_CamView(i_view)
	, m_CamProj(i_proj)
	, m_KeyStates(0)
{
	m_LeftDir = floral::normalize(floral::cross(m_CamView.up_direction, m_CamView.look_at));
	_UpdateMatrices();
}

void FreeCamera::OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus)
{
	switch (i_keyCode)
	{
		case 0x41:								// A key
			if (i_keyStatus == 0)
			{
				SET_BIT(m_KeyStates, 0x01);
			}
			else if (i_keyStatus == 2)
			{
				CLEAR_BIT(m_KeyStates, 0x01);
			}
			break;
		case 0x53:								// S key
			if (i_keyStatus == 0)
			{
				SET_BIT(m_KeyStates, 0x02);
			}
			else if (i_keyStatus == 2)
			{
				CLEAR_BIT(m_KeyStates, 0x02);
			}
			break;
		case 0x44:								// D key
			if (i_keyStatus == 0)
			{
				SET_BIT(m_KeyStates, 0x04);
			}
			else if (i_keyStatus == 2)
			{
				CLEAR_BIT(m_KeyStates, 0x04);
			}
			break;
		case 0x57:								// W key
			if (i_keyStatus == 0)
			{
				SET_BIT(m_KeyStates, 0x08);
			}
			else if (i_keyStatus == 2)
			{
				CLEAR_BIT(m_KeyStates, 0x08);
			}
			break;
		default:
			break;
	}
}

void FreeCamera::OnCursorMove(const u32 i_x, const u32 i_y)
{
}

void FreeCamera::OnCursorInteract(const bool i_pressed)
{
}

void FreeCamera::OnUpdate(const f32 i_deltaMs)
{
	if (m_KeyStates == 0)
		return;

	floral::vec3f moveDir(0.0f);
	f32 speedFactor = k_SpeedMultiplier * k_CameraSpeed * i_deltaMs;
	if (TEST_BIT(m_KeyStates, 0x01))			// A
		moveDir += m_LeftDir;
	if (TEST_BIT(m_KeyStates, 0x02))			// S
		moveDir += -m_Forward;
	if (TEST_BIT(m_KeyStates, 0x04))			// D
		moveDir += -m_LeftDir;
	if (TEST_BIT(m_KeyStates, 0x08))			// W
		moveDir += m_Forward;

	if (floral::length(moveDir) < 0.01f) return; // to prevent normalize() from NaN, lol!!!

	moveDir = floral::normalize(moveDir) * speedFactor;
	m_CamView.position += floral::normalize(moveDir);

	_UpdateMatrices();
}

void FreeCamera::SetProjection(const f32 i_near, const f32 i_far, const f32 i_fov, const f32 i_ratio)
{
	m_CamProj.near_plane = i_near;
	m_CamProj.far_plane = i_far;
	m_CamProj.fov = i_fov;
	m_CamProj.aspect_ratio = i_ratio;
}

void FreeCamera::_UpdateMatrices()
{
	m_Projection = floral::construct_perspective(m_CamProj);
	m_View = floral::construct_lookat_dir(m_CamView);
}

}
