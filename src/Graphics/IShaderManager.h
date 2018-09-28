#pragma once

#include <floral.h>
#include <insigne/commons.h>

namespace stone {

	class IShaderManager {
		public:
			virtual void							Initialize() = 0;
			virtual const insigne::shader_handle_t	LoadShader(const_cstr i_shaderPathNoExt,
														insigne::shader_param_list_t* i_paramList) = 0;
			virtual const insigne::shader_handle_t	LoadShader(const floral::path& i_cbShaderPath) = 0;
			virtual const insigne::shader_handle_t	LoadShader2(const floral::path& i_cbShaderPath) = 0;
	};

}
