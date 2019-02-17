#include "FreeCamera.h"

#include <clover.h>
#include <platform/windows/event_defs.h>

namespace stone
{

static const f32 k_CameraSpeed = 5.0f;
static const f32 k_SpeedMultiplier = 0.01f;

FreeCamera::FreeCamera(const floral::vec3f& i_position, const floral::vec3f i_upDir, const floral::vec3f i_fwDir)
	: m_Position(i_position)
	, m_UpDir(floral::normalize(i_upDir))
	, m_Forward(floral::normalize(i_fwDir))
	, m_KeyStates(0)
{
	m_LeftDir = floral::normalize(floral::cross(m_UpDir, m_Forward));
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
	m_Position += floral::normalize(moveDir);
}

}
