#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "Component.h"
#include "Graphics/IMaterial.h"

namespace stone {
	class VisualComponent : public Component {
		public:
			VisualComponent();
			~VisualComponent();

			void								Update(f32 i_deltaMs);
			void								Render();

			void								Initialize(insigne::surface_handle_t i_surfaceHdl, IMaterial* i_matHdl);

			void								SetPosition(const floral::vec3f& i_posWS)		{ m_PositionWS = i_posWS; }
			void								SetRotation(const floral::vec3f& i_rotWS)		{ m_RotationWS = i_rotWS; }
			void								SetScaling(const floral::vec3f& i_sclWS)		{ m_ScalingWS = i_sclWS; }

			const floral::vec3f					GetPosition()									{ return m_PositionWS; }
			const floral::vec3f					GetRotation()									{ return m_RotationWS; }
			const floral::vec3f					GetScaling()									{ return m_ScalingWS; }

			IMaterial*							GetMaterial()									{ return m_Material; }

		private:
			insigne::surface_handle_t			m_Surface;
			IMaterial*							m_Material;

			floral::vec3f						m_PositionWS;
			floral::vec3f						m_RotationWS;
			floral::vec3f						m_ScalingWS;
			floral::mat4x4f						m_Transform;
	};
}
