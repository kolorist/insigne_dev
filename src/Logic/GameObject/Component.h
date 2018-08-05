#pragma once

#include <floral.h>

#include "IComponent.h"

namespace stone {

	struct Camera;

	class Component : public IComponent {
		public:
			virtual void						Update(Camera* i_camera, f32 i_deltaMs) override;
			virtual const floral::crc_string&	GetName() override;
	};
}
