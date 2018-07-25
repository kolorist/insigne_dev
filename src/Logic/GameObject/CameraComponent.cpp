#include "CameraComponent.h"

namespace stone {
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

	void CameraComponent::Update(f32 i_deltaMs)
	{
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
