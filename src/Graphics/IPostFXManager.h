#pragma once

#include <floral.h>
#include <insigne/commons.h>

namespace stone {
	class IPostFXManager {
		public:
			virtual void						Initialize() = 0;
			virtual const insigne::framebuffer_handle_t	GetMainFramebuffer() = 0;
			virtual void						Render() = 0;
	};
}
