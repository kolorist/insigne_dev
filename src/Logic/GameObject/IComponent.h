#pragma once

#include <floral.h>

namespace stone {
	class IComponent {
		public:
			virtual void						Update(f32 i_deltaMs) = 0;
			virtual const floral::crc_string&	GetName() = 0;
	};
}
