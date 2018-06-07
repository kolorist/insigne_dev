#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "IMaterial.h"

namespace stone {
	class FBODebugMaterial : public IMaterial {
		public:
			void								Initialize(insigne::shader_handle_t i_shaderHdl) override;
			static insigne::shader_param_list_t*	BuildShaderParamList();

			void								SetColorTex0(insigne::texture_handle_t i_tex);

		private:
			insigne::param_id					m_ColorTex0;
	};
}
