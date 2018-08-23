#include "ShaderManager.h"

#include <cstdio>

#include <insigne/render.h>

int yylex_cbshdr(const char* i_input);

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
	memset(vertSource, 0, shaderFile.file_size);
	floral::read_all_file(shaderFile, vertSource);
	floral::close_file(shaderFile);

	shaderFile = floral::open_file(fragPath);
	cstr fragSource = (cstr)m_MemoryArena->allocate(shaderFile.file_size + 1);
	memset(fragSource, 0, shaderFile.file_size);
	floral::read_all_file(shaderFile, fragSource);
	floral::close_file(shaderFile);

	insigne::shader_handle_t newShader = insigne::compile_shader(vertSource, fragSource, i_paramList);

	m_MemoryArena->free_all();

	return newShader;
}

const insigne::shader_handle_t ShaderManager::LoadShader(const_cstr i_cbShaderPath)
{
	floral::file_info shaderFile = floral::open_file(i_cbShaderPath);
	cstr cbShaderSource = (cstr)m_MemoryArena->allocate(shaderFile.file_size + 1);
	memset(cbShaderSource, 0, shaderFile.file_size);
	floral::read_all_file(shaderFile, cbShaderSource);
	floral::close_file(shaderFile);

	yylex_cbshdr(cbShaderSource);

	return 0;
}

}
