#pragma once

#include <floral.h>

namespace stone {
	struct Camera;

	class IComponent {
		public:
			virtual void						Update(Camera* i_camera, f32 i_deltaMs) = 0;
			virtual const floral::crc_string&	GetName() = 0;
	};
}
