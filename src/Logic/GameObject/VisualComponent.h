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

		private:
			insigne::surface_handle_t			m_Surface;
			IMaterial*							m_Material;
	};
}
