#pragma once

#include <floral.h>

#include "IComponent.h"

namespace stone {
	class IGameObject {
		public:

			virtual void						Update(f32 i_deltaMs) = 0;

			virtual void						AddComponent(IComponent* i_comp) = 0;
			virtual IComponent*					GetComponentByName(const_cstr i_name) = 0;
	};
}
