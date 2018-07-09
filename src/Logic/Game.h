#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "Memory/MemorySystem.h"
#include "GameObject/IGameObject.h"
#include "GameObject/VisualComponent.h"
#include "Graphics/IModelManager.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/ITextureManager.h"

namespace stone {
	class Game {
		public:
			Game(IModelManager* i_modelManager, MaterialManager* i_materialManager, ITextureManager* i_textureManager);
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
			insigne::surface_handle_t			m_SkyboxSurface;

		private:
			IModelManager*						m_ModelManager;
			MaterialManager*					m_MaterialManager;
			ITextureManager*					m_TextureManager;
	};
}
