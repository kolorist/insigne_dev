#pragma once

#include <floral.h>

#include "IComponent.h"

namespace stone {
	class Component : public IComponent {
		public:
			virtual void						Update(f32 i_deltaMs) override;
			virtual const floral::crc_string&	GetName() override;
	};
}
