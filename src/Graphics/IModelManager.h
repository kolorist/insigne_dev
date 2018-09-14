#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "RenderData.h"

namespace stone {

class IModelManager {
	public:
		virtual void						Initialize() = 0;
		virtual insigne::surface_handle_t	CreateSingleSurface(const_cstr i_surfPath) = 0;

		virtual Model*						CreateModel(const floral::path& i_path, floral::aabb3f& o_aabb) = 0;
};

}
