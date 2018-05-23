#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "IMaterial.h"

namespace stone {
	class PBRMaterial : public IMaterial {
		public:
			void								Initialize(insigne::shader_handle_t i_shaderHdl) override;

			static insigne::shader_param_list_t*	BuildShaderParamList();
	};
}
