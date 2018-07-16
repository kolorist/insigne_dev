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
	class Debugger;
	class IMaterial;
}

namespace stone {
	class Game {
		public:
			Game(IModelManager* i_modelManager, MaterialManager* i_materialManager, ITextureManager* i_textureManager,
					Debugger* i_debugger);
			~Game();

			void								Initialize();

			void								Update(f32 i_deltaMs);
			void								Render();

		private:
			void								RequestLoadDefaultTextures();
			void								RequestLoadPlateMaterial();
			void								RequestLoadModels();
			void								RequestLoadAndApplyTextures();
			void								RequestLoadSkybox();
			void								RequestLoadShadingProbes();

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
			Debugger*							m_Debugger;

		private:
			insigne::texture_handle_t			m_DefaultAlbedo;
			insigne::texture_handle_t			m_DefaultMetallic;
			insigne::texture_handle_t			m_DefaultRoughness;

			insigne::texture_handle_t			m_Albedo;
			insigne::texture_handle_t			m_Metallic;
			insigne::texture_handle_t			m_Rougness;
			insigne::texture_handle_t			m_AO;
			insigne::texture_handle_t			m_Normal;

			insigne::texture_handle_t			m_SkyboxAlbedo;

			IMaterial*							m_PlateMaterial;
			IMaterial*							m_SkyboxMaterial;
			insigne::surface_handle_t			m_PlateModel;
			insigne::surface_handle_t			m_SkyboxModel;
	};
}
