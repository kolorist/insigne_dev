#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "Component.h"
#include "Graphics/IMaterial.h"

namespace stone {

struct Camera;
struct Model;

class VisualComponent : public Component {
	public:
		VisualComponent();
		~VisualComponent();

		void								Update(Camera* i_camera, f32 i_deltaMs);
		void								Render(Camera* i_camera);
		void								RenderWithMaterial(Camera* i_camera, insigne::material_handle_t i_ovrMaterial);

		void								Initialize(Model* i_model);

		void								SetPosition(const floral::vec3f& i_posWS)		{ m_PositionWS = i_posWS; }
		void								SetRotation(const floral::vec3f& i_rotWS)		{ m_RotationWS = i_rotWS; }
		void								SetScaling(const floral::vec3f& i_sclWS)		{ m_ScalingWS = i_sclWS; }

		const floral::vec3f					GetPosition()									{ return m_PositionWS; }
		const floral::vec3f					GetRotation()									{ return m_RotationWS; }
		const floral::vec3f					GetScaling()									{ return m_ScalingWS; }

	private:
		floral::vec3f						m_PositionWS;
		floral::vec3f						m_RotationWS;
		floral::vec3f						m_ScalingWS;
		floral::mat4x4f						m_Transform;

		Model*									m_Model;
};
}
