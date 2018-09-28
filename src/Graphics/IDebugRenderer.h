#pragma once

#include <floral.h>

namespace stone
{

struct Camera;

class IDebugRenderer {
	public:
		virtual void Initialize() = 0;
		virtual void DrawLine3D(const floral::vec3f& i_x0, const floral::vec3f& i_x1, const floral::vec4f& i_color) = 0;
		virtual void Render(Camera* i_camera) = 0;
};

}
