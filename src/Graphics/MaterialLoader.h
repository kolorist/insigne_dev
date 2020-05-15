#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "MaterialParser.h"

namespace mat_loader
{
// ----------------------------------------------------------------------------

struct MaterialShaderPair
{
	insigne::shader_handle_t					shader;
	insigne::material_desc_t					material;
};

template <class TIOAllocator, class TMemoryAllocator>
const bool										CreateMaterial(MaterialShaderPair* o_mat, const mat_parser::MaterialDescription& i_matDesc, TMemoryAllocator* i_ioAllocator, TMemoryAllocator* i_dataAllocator);
template <class TMemoryAllocator>
const bool										CreateMaterial(MaterialShaderPair* o_mat, const mat_parser::MaterialDescription& i_matDesc, nullptr_t i_ioAllocator, TMemoryAllocator* i_dataAllocator);
template <class TFileSystem, class TIOAllocator, class TMemoryAllocator>
const bool										CreateMaterial(MaterialShaderPair* o_mat, TFileSystem* i_fs, const mat_parser::MaterialDescription& i_matDesc, TIOAllocator* i_ioAllocator, TMemoryAllocator* i_dataAllocator);

namespace internal
{
// ----------------------------------------------------------------------------

template <class TMemoryAllocator>
const insigne::ub_handle_t						BuildUniformBuffer(const mat_parser::UBDescription& i_ubDesc, TMemoryAllocator* i_dataAllocator);

// ----------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
}

#include "MaterialLoader.inl"
