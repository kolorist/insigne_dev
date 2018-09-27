#include "CameraComponent.h"

namespace stone {

static const f32 k_camMoveSpeed = 0.001f;

CameraComponent::CameraComponent()
{
}

CameraComponent::~CameraComponent()
{
}

void CameraComponent::Initialize(const f32 i_nearPlane, const f32 i_farPlane, const f32 i_fov, const f32 i_ratio)
{
	m_Camera.NearPlane = i_nearPlane;
	m_Camera.FarPlane = i_farPlane;
	m_Camera.FOV = i_fov;
	m_Camera.AspectRatio = i_ratio;

	m_isDirty = true;
}

void CameraComponent::Update(Camera* i_camera, f32 i_deltaMs, const u32 i_camKeyState)
{
	if (i_camKeyState) {
		floral::vec3f forwardDir = m_Camera.LookAtDir.normalize();
		floral::vec3f backwardDir = -forwardDir;
		floral::vec3f upDir(0.0f, 1.0f, 0.0f);
		floral::vec3f leftDir = upDir.cross(forwardDir).normalize();
		floral::vec3f rightDir = -leftDir;

		f32 deltaDist = i_deltaMs * k_camMoveSpeed;

		if (TEST_BIT(i_camKeyState, 0x1))
			m_Camera.Position += forwardDir * deltaDist;

		if (TEST_BIT(i_camKeyState, 0x2))
			m_Camera.Position += backwardDir * deltaDist;

		if (TEST_BIT(i_camKeyState, 0x4))
			m_Camera.Position += leftDir * deltaDist;

		if (TEST_BIT(i_camKeyState, 0x8))
			m_Camera.Position += rightDir * deltaDist;

		m_isDirty = true;
	}

	if (m_isDirty) {
		m_Camera.ViewMatrix = floral::construct_lookat(
				floral::vec3f(0.0f, 1.0f, 0.0f),
				m_Camera.Position,
				m_Camera.LookAtDir);
		m_Camera.ProjectionMatrix = floral::construct_perspective(
				m_Camera.NearPlane,
				m_Camera.FarPlane,
				m_Camera.FOV,
				m_Camera.AspectRatio);
		m_Camera.WVPMatrix = m_Camera.ProjectionMatrix * m_Camera.ViewMatrix;
		m_isDirty = false;
	}
}
}
