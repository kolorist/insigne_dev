#pragma once

#include <floral.h>

#include "IShaderManager.h"
#include "Memory/MemorySystem.h"

namespace stone {

	class ShaderManager : public IShaderManager {
		public:
			ShaderManager();
			~ShaderManager();

			void								Initialize() override;
			const insigne::shader_handle_t		LoadShader(const_cstr i_shaderPathNoExt, insigne::shader_param_list_t* i_paramList) override;
			const insigne::shader_handle_t		LoadShader(const_cstr i_cbShaderPath) override;

		private:
			struct ShaderRegistry {
				floral::crc_string				CRCPath;
				insigne::shader_handle_t		ShaderHandle;
			};

		private:
			floral::fixed_array<ShaderRegistry, LinearAllocator>	m_PersistanceShaderDB;
			floral::fixed_array<ShaderRegistry, LinearAllocator>	m_SceneShaderDB;

			LinearArena*						m_MemoryArena;
	};

}
