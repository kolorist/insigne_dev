#pragma once

#include <floral.h>
#include <insigne/commons.h>

namespace stone {

class ILightingManager {
	public:
		virtual void							Initialize() = 0;
		virtual void							RenderShadowMap() = 0;

		virtual insigne::texture_handle_t		GetShadowMap() = 0;
};

}
