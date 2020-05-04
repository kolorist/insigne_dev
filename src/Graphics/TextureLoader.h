#pragma once

#include <floral/stdaliases.h>
#include <floral/gpds/vec.h>

#include <insigne/commons.h>

namespace tex_loader
{
// ----------------------------------------------------------------------------

template <class TIOAllocator>
const insigne::texture_handle_t					LoadCBTexture(const floral::path& i_path, insigne::texture_desc_t& io_desc, TIOAllocator* i_ioAllocator, const bool i_loadMipmaps = false);

// LDR TGA texture only, use stb_image and stb_image_resize, slow
const insigne::texture_handle_t					LoadLDRTexture2D(const floral::path& i_path, insigne::texture_desc_t& io_desc, const bool i_createMipmaps = false);
const insigne::texture_handle_t					LoadLDRTextureCube(const floral::path& i_path, insigne::texture_desc_t& io_desc, const bool i_createMipmaps = false);

const insigne::texture_handle_t					LoadHDRTexture2D(const floral::path& i_path, insigne::texture_desc_t& io_desc, const bool i_createMipmaps = false);
const insigne::texture_handle_t					LoadHDRTextureCube(const floral::path& i_path, insigne::texture_desc_t& io_desc, const bool i_createMipmaps = false);

struct PBREnv
{
	floral::vec3f								shCoeffs[9];
	insigne::texture_handle_t					specularCubeTexture;
};

template <class TAllocator>
const PBREnv									LoadPBREnvTexture(TAllocator* i_allocator);

namespace internal
{
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
}

#include "TextureLoader.inl"
