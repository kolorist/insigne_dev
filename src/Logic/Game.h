#pragma once

#include <floral.h>

#include "Memory/MemorySystem.h"
#include "GameObject/IGameObject.h"
#include "GameObject/VisualComponent.h"
#include "Graphics/IModelManager.h"
#include "Graphics/MaterialManager.h"

namespace stone {
	class Game {
		public:
			Game(IModelManager* i_modelManager, MaterialManager* i_materialManager);
			~Game();

			void								Initialize();

			void								Update(f32 i_deltaMs);
			void								Render();

		private:
			typedef floral::fixed_array<IGameObject*, LinearAllocator>		GameObjectArray;
			typedef floral::fixed_array<VisualComponent*, LinearAllocator>	VisualComponentArray;

		private:
			GameObjectArray*					m_GameObjects;
			VisualComponentArray*				m_VisualComponents;

		private:
			IModelManager*						m_ModelManager;
			MaterialManager*					m_MaterialManager;
	};
}
