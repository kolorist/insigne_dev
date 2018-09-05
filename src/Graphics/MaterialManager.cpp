#include "MaterialManager.h"

#include "CBRenderDescs.h"

#include <insigne/render.h>

//parser
int yylex_cbmat(const_cstr i_input, cymbi::MaterialDesc& o_matDesc);

namespace stone {

MaterialManager::MaterialManager(IShaderManager* i_shaderManager,
		ITextureManager* i_textureManager)
	: m_ShaderManager(i_shaderManager)
	, m_TextureManager(i_textureManager)
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
	for (u32 i = 0; i < matDesc.tex2DParams.get_size(); i++) {
		insigne::texture_handle_t texHdl = m_TextureManager->CreateTexture(matDesc.tex2DParams[i].value);
		insigne::param_id texParamId = insigne::get_material_param<insigne::texture_handle_t>(matHdl, matDesc.tex2DParams[i].name);
		insigne::set_material_param(matHdl, texParamId, texHdl);
	}

	for (u32 i = 0; i < matDesc.texCubeParams.get_size(); i++) {
		insigne::texture_handle_t texHdl = m_TextureManager->CreateMipmapedProbe(matDesc.texCubeParams[i].value);
		insigne::param_id texParamId = insigne::get_material_param_texcube(matHdl, matDesc.texCubeParams[i].name);
		insigne::set_material_param_texcube(matHdl, texParamId, texHdl);
	}

	for (u32 i = 0; i < matDesc.floatParams.get_size(); i++) {
		insigne::param_id pId = insigne::get_material_param<f32>(matHdl, matDesc.floatParams[i].name);
		insigne::set_material_param(matHdl, pId, matDesc.floatParams[i].value);
	}

	for (u32 i = 0; i < matDesc.vec3Params.get_size(); i++) {
		insigne::param_id pId = insigne::get_material_param<floral::vec3f>(matHdl, matDesc.vec3Params[i].name);
		insigne::set_material_param(matHdl, pId, matDesc.vec3Params[i].value);
	}

	return matHdl;
}

}
