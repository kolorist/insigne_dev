#include "ShaderManager.h"

#include <cstdio>
#include <clover.h>

#include <insigne/render.h>
#include <insigne/ut_shading.h>

#include "CBRenderDescs.h"

// parser
int yylex_cbshdr(const char* i_input, cymbi::ShaderDesc& o_shaderDesc);

namespace stone {

ShaderManager::ShaderManager()
{
	m_MemoryArena = g_PersistanceAllocator.allocate_arena<LinearArena>(SIZE_MB(4));
}

ShaderManager::~ShaderManager()
{
	// TODO
}

void ShaderManager::Initialize()
{
	m_PersistanceShaderDB.init(16u, &g_PersistanceResourceAllocator);
}

const insigne::shader_handle_t ShaderManager::LoadShader(const_cstr i_shaderPathNoExt,
		insigne::shader_param_list_t* i_paramList)
{
	// TODO: load from cache
	
	c8 vertPath[1024];
	c8 fragPath[1024];

	sprintf(vertPath, "%s%s", i_shaderPathNoExt, ".vert");
	sprintf(fragPath, "%s%s", i_shaderPathNoExt, ".frag");

	floral::file_info shaderFile = floral::open_file(vertPath);
	cstr vertSource = (cstr)m_MemoryArena->allocate(shaderFile.file_size + 1);
	memset(vertSource, 0, shaderFile.file_size + 1);
	floral::read_all_file(shaderFile, vertSource);
	floral::close_file(shaderFile);

	shaderFile = floral::open_file(fragPath);
	cstr fragSource = (cstr)m_MemoryArena->allocate(shaderFile.file_size + 1);
	memset(fragSource, 0, shaderFile.file_size + 1);
	floral::read_all_file(shaderFile, fragSource);
	floral::close_file(shaderFile);

	insigne::shader_handle_t newShader = insigne::compile_shader(vertSource, fragSource, i_paramList);

	m_MemoryArena->free_all();

	return newShader;
}

const insigne::shader_handle_t ShaderManager::LoadShader(const floral::path& i_cbShaderPath)
{
	floral::file_info shaderFile = floral::open_file(i_cbShaderPath);
	cstr cbShaderSource = (cstr)m_MemoryArena->allocate(shaderFile.file_size + 1);
	memset(cbShaderSource, 0, shaderFile.file_size + 1);
	floral::read_all_file(shaderFile, cbShaderSource);
	floral::close_file(shaderFile);

	cymbi::ShaderDesc shaderDesc;
	// build shader param list
	shaderDesc.shaderParams = insigne::allocate_shader_param_list(32);
	yylex_cbshdr(cbShaderSource, shaderDesc);

	shaderFile = floral::open_file(shaderDesc.vertexShaderPath);
	cstr vertSource = (cstr)m_MemoryArena->allocate(shaderFile.file_size + 1);
	memset(vertSource, 0, shaderFile.file_size + 1);
	floral::read_all_file(shaderFile, vertSource);
	floral::close_file(shaderFile);

	shaderFile = floral::open_file(shaderDesc.fragmentShaderPath);
	cstr fragSource = (cstr)m_MemoryArena->allocate(shaderFile.file_size + 1);
	memset(fragSource, 0, shaderFile.file_size + 1);
	floral::read_all_file(shaderFile, fragSource);
	floral::close_file(shaderFile);

	insigne::shader_handle_t newShader = insigne::compile_shader(vertSource, fragSource, shaderDesc.shaderParams);
	CLOVER_DEBUG("Shader %d: %s", newShader, i_cbShaderPath.pm_PathStr);

	m_MemoryArena->free_all();

	return newShader;
}

const insigne::shader_handle_t ShaderManager::LoadShader2(const floral::path& i_cbShaderPath)
{
	floral::file_info shaderFile = floral::open_file(i_cbShaderPath);
	cstr cbShaderSource = (cstr)m_MemoryArena->allocate(shaderFile.file_size + 1);
	memset(cbShaderSource, 0, shaderFile.file_size + 1);
	floral::read_all_file(shaderFile, cbShaderSource);
	floral::close_file(shaderFile);

	cymbi::ShaderDesc shaderDesc;
	// build shader param list
	shaderDesc.shaderParams = insigne::allocate_shader_param_list(32);
	yylex_cbshdr(cbShaderSource, shaderDesc);

	insigne::shader_desc_t desc = insigne::create_shader_desc();
	desc.vs_path = shaderDesc.vertexShaderPath;
	desc.fs_path = shaderDesc.fragmentShaderPath;

	{
		for (u32 i = 0; i < shaderDesc.shaderParams->get_size(); i++) {
			const insigne::shader_param_t& sparam = shaderDesc.shaderParams->at(i);
			switch (sparam.data_type) {
				case insigne::param_data_type_e::param_sampler2d:
				case insigne::param_data_type_e::param_sampler_cube:
					desc.reflection.textures->push_back(sparam);
					break;
				case insigne::param_data_type_e::param_ub:
					desc.reflection.uniform_blocks->push_back(sparam);
					break;
				default:
					break;
			}
		}
	}

	shaderFile = floral::open_file(shaderDesc.vertexShaderPath);
	memset(desc.vs, 0, shaderFile.file_size + 1);
	floral::read_all_file(shaderFile, desc.vs);
	floral::close_file(shaderFile);

	shaderFile = floral::open_file(shaderDesc.fragmentShaderPath);
	memset(desc.fs, 0, shaderFile.file_size + 1);
	floral::read_all_file(shaderFile, desc.fs);
	floral::close_file(shaderFile);

	insigne::shader_handle_t newShader = insigne::create_shader(desc);
	insigne::material_desc_t newMaterial;
	insigne::infuse_material(newShader, newMaterial);

	m_MemoryArena->free_all();

	return newShader;
}

}
