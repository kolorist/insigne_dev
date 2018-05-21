#pragma once

#include <floral.h>

#include "IShaderManager.h"

namespace stone {

	class MaterialManager {
		public:
			MaterialManager(IShaderManager* i_shaderManager);
			~MaterialManager();

		public:
			template <typename TMaterial>
			TMaterial*							CreateMaterial(const_cstr i_matPath);

		private:
			IShaderManager*						m_ShaderManager;
	};

}

#include "MaterialManager.hpp"
