#include "MaterialManager.h"

#include "CBRenderDescs.h"

#include <insigne/render.h>

//parser
int yylex_cbmat(const_cstr i_input, cymbi::MaterialDesc& o_matDesc);

namespace stone {
	MaterialManager::MaterialManager(IShaderManager* i_shaderManager)
		: m_ShaderManager(i_shaderManager)
	{
		m_MemoryArena = g_PersistanceAllocator.allocate_arena<LinearArena>(SIZE_MB(4));
	}

	MaterialManager::~MaterialManager()
	{
	}

	insigne::material_handle_t MaterialManager::CreateMaterialFromFile(const floral::path& i_matPath)
	{
		floral::file_info materialFile = floral::open_file(i_matPath);
		cstr matSource = (cstr)m_MemoryArena->allocate(materialFile.file_size + 1);
		memset(matSource, 0, materialFile.file_size);
		floral::read_all_file(materialFile, matSource);
		floral::close_file(materialFile);

		cymbi::MaterialDesc matDesc;
		yylex_cbmat(matSource, matDesc);

		insigne::shader_handle_t shaderHdl = m_ShaderManager->LoadShader(matDesc.cbShaderPath);
		insigne::material_handle_t matHdl = insigne::create_material(shaderHdl);

		// params setup

		return matHdl;
	}
}
