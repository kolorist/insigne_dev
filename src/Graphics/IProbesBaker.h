#pragma once
#include <insigne/commons.h>

namespace stone {

class IProbesBaker {
	public:
		virtual void							Initialize() = 0;
		virtual void							Render() = 0;

		virtual insigne::framebuffer_handle_t	GetMegaFramebuffer() = 0;
};

}
