#include "TrackballCamera.h"

#include <clover.h>

#include <math.h>

namespace stone {

TrackballCamera::TrackballCamera(const floral::camera_view_t& i_view, const floral::camera_persp_t& i_proj)
	: m_CursorPressed(false)
	, m_PrevCursorPos(0, 0)
	, m_CursorPos(0, 0)
	, m_Radius(0.0f)
	, m_CamView(i_view)
{
	m_Projection = floral::construct_perspective(i_proj);
}

void TrackballCamera::SetScreenResolution(const u32 i_width, const u32 i_height)
{
	m_HalfWidth = i_width / 2;
	m_HalfHeight = i_height / 2;
	if (m_HalfWidth > m_HalfHeight)
	{
		m_Radius = (f32)m_HalfHeight;
	}
	else
	{
		m_Radius = (f32)m_HalfWidth;
	}
}

floral::mat4x4f TrackballCamera::GetWVP() const
{
	floral::quaternionf q = GetRotation();

	floral::camera_view_t cv = m_CamView;
	floral::vec4f cp = floral::vec4f(cv.position, 1.0f);
	floral::vec4f cu = floral::vec4f(cv.up_direction, 0.0f);
	floral::mat4x4f m = q.to_transform();
	cp = m * cp;
	cu = m * cu;
	cv.position = floral::vec3f(cp.x, cp.y, cp.z);
	cv.up_direction = floral::vec3f(cu.x, cu.y, cu.z);

	floral::mat4x4f v = floral::construct_lookat_point(cv);
	return m_Projection * v;
}

floral::vec3f TrackballCamera::GetPosition() const
{
	floral::quaternionf q = GetRotation();

	floral::camera_view_t cv = m_CamView;
	floral::vec4f cp = floral::vec4f(cv.position, 1.0f);
	floral::mat4x4f m = q.to_transform();
	cp = m * cp;
	return floral::vec3f(cp.x, cp.y, cp.z);
}

void TrackballCamera::OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus)
{
}

void TrackballCamera::OnCursorMove(const u32 i_x, const u32 i_y)
{
	m_CursorPos = floral::vec2i(i_x, i_y);
}

void TrackballCamera::OnCursorInteract(const bool i_pressed)
{
	if (i_pressed)
	{
		m_CursorPressed = true;
		m_PrevCursorPos = m_CursorPos;
	}
	else
	{
		m_CursorPressed = false;
		m_PrevRotation = (m_Rotation * m_PrevRotation).normalize();
		m_Rotation = floral::quaternionf();
	}
}

void TrackballCamera::OnUpdate(const f32 i_deltaMs)
{
	if (m_CursorPressed)
	{
		floral::vec3f prevArc = _GetArcCursor(m_PrevCursorPos);
		floral::vec3f arc = _GetArcCursor(m_CursorPos);
		m_Rotation = floral::construct_quaternion_v2v(prevArc, arc).normalize();
	}
}

//----------------------------------------------

floral::vec3f TrackballCamera::_GetArcCursor(const floral::vec2i& i_vec) const
{
	if (m_HalfWidth == 0 || m_HalfHeight == 0 || m_Radius == 0.0f)
	{
		return floral::vec3f(0.0f);
	}

	f32 x = i_vec.x - m_HalfWidth;
	f32 y = m_HalfHeight - i_vec.y;

	f32 arc = sqrtf(x * x + y * y);
	f32 a = (arc / m_Radius);
	f32 b = atan2f(y, x);
	f32 x2 = m_Radius * sinf(a);

	floral::vec3f ret;
	ret.z = x2 * cosf(b);
	ret.y = -x2 * sinf(b);
	ret.x = m_Radius * cosf(a);

	return floral::normalize(ret);
}

}
