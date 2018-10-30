#include "TrackballCamera.h"

#include <clover.h>

namespace stone {

TrackballCamera::TrackballCamera()
{
	m_Rotation = floral::construct_quaternion_axis(floral::vec3f(0.0f, 0.0f, 0.0f), 0.0f);
	m_LastRotation = floral::construct_quaternion_axis(floral::vec3f(0.0f, 0.0f, 0.0f), 0.0f);

	floral::camera_view_t camView;
	camView.position = floral::normalize(floral::vec3f(7.0f, 0.5f, 0.0f));
	camView.look_at = floral::vec3f(0.0f);
	camView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

	m_InverseView = floral::construct_lookat_point(camView).get_inverse();
}

void TrackballCamera::OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus)
{
}

void TrackballCamera::OnCursorMove(const u32 i_x, const u32 i_y)
{
	f32 x = (f32)i_x / 1280.0f * 2.0f - 1.0f;
	f32 y = -((f32)i_y / 720.0f * 2.0f - 1.0f);
	m_SphereCoord.x = x;
	m_SphereCoord.y = y;

	if (x * x + y * y >= 0.5f) {
		m_SphereCoord.z = 0.5f / sqrtf(x * x + y * y);
	} else {
		m_SphereCoord.z = sqrtf(1.0f - (x * x + y * y));
	}

	if (m_CursorPressed) {
		floral::vec4f p(m_SphereCoord.x, m_SphereCoord.y, m_SphereCoord.z, 1.0f);
		p = floral::normalize(m_InverseView * p);
		m_EndCoord = floral::normalize(floral::vec3f(p.x, p.y, p.z));

		floral::vec3f n = floral::cross(m_StartCoord, m_EndCoord);
		f32 theta = floral::to_degree(floral::angle(m_StartCoord, m_EndCoord)) * 6.0f;
		floral::quaternionf q = floral::construct_quaternion_axis(n, theta);
		m_Rotation = q;
	}
}

void TrackballCamera::OnCursorInteract(const bool i_pressed)
{
	if (i_pressed) {
		if (!m_CursorPressed) {
			floral::vec4f p(m_SphereCoord.x, m_SphereCoord.y, m_SphereCoord.z, 1.0f);
			p = m_InverseView * p;
			m_StartCoord = floral::normalize(floral::vec3f(p.x, p.y, p.z));
		}
	} else {
		if (m_CursorPressed) {
			m_LastRotation = m_Rotation * m_LastRotation;
			m_Rotation = floral::construct_quaternion_axis(floral::vec3f(0.0f, 0.0f, 1.0f), 0.0f);
		}
	}
	m_CursorPressed = i_pressed;
}

}
