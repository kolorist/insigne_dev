#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "Graphics/IMaterial.h"

namespace stone {
	class DebugUIMaterial : public IMaterial {
		public:
			void								Initialize(insigne::shader_handle_t i_shaderHdl) override;

			static insigne::shader_param_list_t*	BuildShaderParamList();

			void								SetDebugOrthoWVP(const floral::mat4x4f& i_wvp);
			void								SetTexture(const insigne::texture_handle_t& i_tex);

		private:
			insigne::param_id					m_ParamWVP;
			insigne::param_id					m_ParamTex;
	};
}
