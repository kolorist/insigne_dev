#include "Memory/MemorySystem.h"

namespace stone {
	template <typename TMaterial>
	TMaterial* MaterialManager::CreateMaterial(const_cstr i_matPath)
	{
		insigne::shader_handle_t shaderHdl = m_ShaderManager->LoadShader(i_matPath, TMaterial::BuildShaderParamList());
		TMaterial* newMaterial = g_PersistanceResourceAllocator.allocate<TMaterial>();
		newMaterial->Initialize(shaderHdl);

		return newMaterial;
	}
}
