#pragma once

#include <floral.h>

#include "Component.h"
#include "Graphics/Camera.h"

namespace stone {
class CameraComponent : public Component {
	public:
		CameraComponent();
		~CameraComponent();

		void								Update(Camera* i_camera, f32 i_deltaMs);
		void								Render();

		void								Initialize(const f32 i_nearPlane, const f32 i_far, const f32 i_fov, const f32 i_ratio);

		void								SetPosition(const floral::vec3f& i_posWS)		{ m_Camera.Position = i_posWS; m_isDirty = true; }
		void								SetLookAtDir(const floral::vec3f& i_dirWS)		{ m_Camera.LookAtDir = i_dirWS; m_isDirty = true; }

		Camera*								GetCamera()										{ return &m_Camera; }

	private:
		Camera								m_Camera;
		bool								m_isDirty;
};
}
