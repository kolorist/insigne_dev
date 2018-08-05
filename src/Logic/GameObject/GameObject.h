#pragma once

#include <floral.h>

#include "IGameObject.h"

namespace stone {
	struct Camera;

	class GameObject : public IGameObject {
		private:
			typedef floral::inplace_array<IComponent*, 4u>	ComponentArray;

		public:
			GameObject();
			~GameObject();

			void								Update(Camera* i_camera, f32 i_deltaMs);

			void								AddComponent(IComponent* i_comp);
			IComponent*							GetComponentByName(const_cstr i_name);

		private:
			ComponentArray						m_Components;
	};
}
