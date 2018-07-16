#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "Component.h"
#include "Graphics/IMaterial.h"

namespace stone {
	class SkyboxComponent : public Component {
		public:
			SkyboxComponent();
			~SkyboxComponent();

			void								Initialize(insigne::surface_handle_t i_surfaceHdl, IMaterial* i_matHdl);
			void								Update(f32 i_deltaMs) override;
			void								Render();

		private:
			insigne::surface_handle_t			m_Surface;
			IMaterial*							m_Material;
	};
}
