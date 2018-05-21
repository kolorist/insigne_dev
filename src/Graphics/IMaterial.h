#pragma once

#include <insigne/commons.h>

namespace stone {
	class IMaterial {
		public:
			virtual void						Initialize(insigne::shader_handle_t i_shaderHdl) = 0;
	};
}
