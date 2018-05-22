#pragma once

#include <floral.h>

#include "Memory/MemorySystem.h"
#include "GameObject/IGameObject.h"
#include "GameObject/VisualComponent.h"

namespace stone {
	class Game {
		public:
			Game();
			~Game();

			void								Update(f32 i_deltaMs);
			void								Render();

		private:
			typedef floral::fixed_array<IGameObject*, LinearAllocator>		GameObjectArray;
			typedef floral::fixed_array<VisualComponent*, LinearAllocator>	VisualComponentArray;

		private:
			GameObjectArray						m_GameObjects;
			VisualComponentArray				m_VisualComponents;
	};
}
