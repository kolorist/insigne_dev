#include "MaterialManager.h"

namespace stone {
	MaterialManager::MaterialManager(IShaderManager* i_shaderManager)
		: m_ShaderManager(i_shaderManager)
	{
	}

	MaterialManager::~MaterialManager()
	{
	}
}
