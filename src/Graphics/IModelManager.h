#pragma once

#include <floral.h>
#include <insigne/commons.h>

namespace stone {
	class IModelManager {
		public:
			virtual void						Initialize() = 0;
			virtual insigne::surface_handle_t	CreateSingleSurface(const_cstr i_surfPath) = 0;
	};
}
