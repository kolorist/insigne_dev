#pragma once
#include <floral.h>
#include <insigne/commons.h>

namespace stone {

struct Camera;

class IProbesBaker {
	public:
		virtual void							Initialize(const floral::aabb3f& i_sceneAABB) = 0;
		virtual void							Render() = 0;
		virtual void							RenderProbes(Camera* i_camera) = 0;
		virtual void							CalculateSHs() = 0;

		virtual insigne::framebuffer_handle_t	GetMegaFramebuffer() = 0;
};

}
