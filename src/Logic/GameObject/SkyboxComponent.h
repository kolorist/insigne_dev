#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "Component.h"
#include "Graphics/IMaterial.h"

namespace stone {
	struct Camera;

	class SkyboxComponent : public Component {
		public:
			SkyboxComponent();
			~SkyboxComponent();

			void								Initialize(insigne::surface_handle_t i_surfaceHdl, IMaterial* i_matHdl);
			void								Update(Camera* i_camera, f32 i_deltaMs) override;
			void								Render(Camera* i_camera);

		private:
			insigne::surface_handle_t			m_Surface;
			IMaterial*							m_Material;

			floral::mat4x4f						m_Transform;
	};
}
