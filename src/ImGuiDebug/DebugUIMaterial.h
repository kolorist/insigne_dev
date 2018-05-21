#pragma once

#include "Graphics/IMaterial.h"

namespace stone {
	class DebugUIMaterial : public IMaterial {
		public:
			void								Initialize(insigne::shader_handle_t i_shaderHdl) override;

			static insigne::shader_param_list_t*	BuildShaderParamList();

		private:
			insigne::material_handle_t			m_MaterialHandle;
	};
}
