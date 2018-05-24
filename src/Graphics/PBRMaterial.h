#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "IMaterial.h"

namespace stone {
	class PBRMaterial : public IMaterial {
		public:
			void								Initialize(insigne::shader_handle_t i_shaderHdl) override;

			static insigne::shader_param_list_t*	BuildShaderParamList();

			void								SetWVP(const floral::mat4x4f& i_wvp);
			void								SetTransform(const floral::mat4x4f& i_xform);

		private:
			insigne::param_id					m_ParamWVP;
			insigne::param_id					m_ParamXformMatrix;
	};
}
