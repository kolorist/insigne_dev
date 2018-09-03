#pragma once

#include <floral.h>

#include "IShaderManager.h"
#include "IMaterialManager.h"
#include "ITextureManager.h"
#include "Memory/MemorySystem.h"

namespace stone {

	class MaterialManager : public IMaterialManager{
		public:
			MaterialManager(IShaderManager* i_shaderManager, ITextureManager* i_textureManager);
			~MaterialManager();

		public:
			template <typename TMaterial>
			TMaterial*							CreateMaterial(const_cstr i_matPath);

			insigne::material_handle_t			CreateMaterialFromFile(const floral::path& i_matPath) override final;

		private:
			IShaderManager*						m_ShaderManager;
			ITextureManager*					m_TextureManager;

			LinearArena*						m_MemoryArena;
	};

}

#include "MaterialManager.hpp"
