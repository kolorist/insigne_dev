#include "FreeCamera.h"

#include <clover.h>
#include <calyx/platform/windows/event_defs.h>

namespace stone
{

static const f32 k_CameraSpeed = 5.0f;
static const f32 k_SpeedDivisor = 5000.0f;

static const f32 k_OrbitalSpeed = 10.0f;
static const f32 k_OrbitalSpeedDivisor = 500.0f;

FreeCamera::FreeCamera(const floral::camera_view_t& i_view, const floral::camera_persp_t& i_proj)
	: m_CursorPos(0.0f)
	, m_CamView(i_view)
	, m_CamProj(i_proj)
	, m_KeyStates(0)
{
	m_CamView.look_at = floral::normalize(m_CamView.look_at);
	m_Orbital = floral::vec3f(
			floral::to_degree(atan2f(m_CamView.look_at.z, m_CamView.look_at.x)),
			floral::to_degree(acosf(m_CamView.look_at.y)),
			0.0f);

	m_CamView.look_at = floral::vec3f(
			sinf(floral::to_radians(m_Orbital.y)) * cosf(floral::to_radians(m_Orbital.x)),
			cosf(floral::to_radians(m_Orbital.y)),
			sinf(floral::to_radians(m_Orbital.x)) * sinf(floral::to_radians(m_Orbital.y))
			);

	m_LeftDir = floral::normalize(floral::cross(m_CamView.up_direction, m_CamView.look_at));
	m_Forward = floral::normalize(floral::cross(m_LeftDir, m_CamView.up_direction));
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
		case 0x49:								// I key
			if (i_keyStatus == 0)
			{
				SET_BIT(m_KeyStates, 0x10);
			}
			else if (i_keyStatus == 2)
			{
				CLEAR_BIT(m_KeyStates, 0x10);
			}
			break;
		case 0x4B:								// K key
			if (i_keyStatus == 0)
			{
				SET_BIT(m_KeyStates, 0x20);
			}
			else if (i_keyStatus == 2)
			{
				CLEAR_BIT(m_KeyStates, 0x20);
			}
			break;
		case 0x4A:								// J key
			if (i_keyStatus == 0)
			{
				SET_BIT(m_KeyStates, 0x40);
			}
			else if (i_keyStatus == 2)
			{
				CLEAR_BIT(m_KeyStates, 0x40);
			}
			break;
		case 0x4C:								// L key
			if (i_keyStatus == 0)
			{
				SET_BIT(m_KeyStates, 0x80);
			}
			else if (i_keyStatus == 2)
			{
				CLEAR_BIT(m_KeyStates, 0x80);
			}
			break;
		default:
			break;
	}
}

void FreeCamera::OnCursorMove(const u32 i_x, const u32 i_y)
{
	m_CursorPos.x = i_x;
	m_CursorPos.y = i_y;
}

void FreeCamera::OnCursorInteract(const bool i_pressed)
{
}

void FreeCamera::OnUpdate(const f32 i_deltaMs)
{
	if (m_KeyStates == 0)
		return;

	floral::vec3f moveDir(0.0f);
	floral::vec3f orbitalDir(0.0f);

	if (TEST_BIT(m_KeyStates, 0x01))			// A
		moveDir += m_LeftDir;
	if (TEST_BIT(m_KeyStates, 0x02))			// S
		moveDir += -m_Forward;
	if (TEST_BIT(m_KeyStates, 0x04))			// D
		moveDir += -m_LeftDir;
	if (TEST_BIT(m_KeyStates, 0x08))			// W
		moveDir += m_Forward;

	f32 orbitalDisp = i_deltaMs * k_OrbitalSpeed / k_OrbitalSpeedDivisor;
	if (TEST_BIT(m_KeyStates, 0x10))			// I
		orbitalDir += floral::vec3f(0.0f, -orbitalDisp, 0.0f);
	if (TEST_BIT(m_KeyStates, 0x20))			// K
		orbitalDir += floral::vec3f(0.0f, orbitalDisp, 0.0f);
	if (TEST_BIT(m_KeyStates, 0x40))			// J
		orbitalDir += floral::vec3f(-orbitalDisp, 0.0f, 0.0f);
	if (TEST_BIT(m_KeyStates, 0x80))			// L
		orbitalDir += floral::vec3f(orbitalDisp, 0.0f, 0.0f);

	bool needUpdateMatrices = false;
	if (floral::length(moveDir) > 0.0f)
	{
		moveDir = floral::normalize(moveDir) * i_deltaMs * k_CameraSpeed / k_SpeedDivisor;
		m_CamView.position += moveDir;
		needUpdateMatrices = true;
	}

	if (floral::length(orbitalDir) > 0.0f)
	{
		m_Orbital += orbitalDir;
		m_CamView.look_at = floral::vec3f(
				sinf(floral::to_radians(m_Orbital.y)) * cosf(floral::to_radians(m_Orbital.x)),
				cosf(floral::to_radians(m_Orbital.y)),
				sinf(floral::to_radians(m_Orbital.x)) * sinf(floral::to_radians(m_Orbital.y))
				);
		needUpdateMatrices = true;
	}

	if (needUpdateMatrices)
	{
		_UpdateMatrices();
		m_LeftDir = floral::normalize(floral::cross(m_CamView.up_direction, m_CamView.look_at));
		m_Forward = floral::normalize(floral::cross(m_LeftDir, m_CamView.up_direction));
	}
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
