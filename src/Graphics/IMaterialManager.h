#pragma once

#include <floral.h>
#include <insigne/commons.h>

namespace stone {

class IMaterialManager {
	public:
		virtual insigne::material_handle_t		CreateMaterialFromFile(const floral::path& i_matPath) = 0;
};

}
