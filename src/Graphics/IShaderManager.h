#pragma once

#include <insigne/commons.h>

namespace stone {

	class IShaderManager {
		public:
			virtual const insigne::shader_handle_t&	LoadShader(const_cstr i_vertPath, const_cstr i_fragPath) = 0;
	};

}
