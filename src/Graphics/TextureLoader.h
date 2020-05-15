#pragma once

#include <floral/stdaliases.h>
#include <floral/gpds/vec.h>
#include <floral/io/filesystem.h>

#include <insigne/commons.h>

namespace tex_loader
{
// ----------------------------------------------------------------------------

enum class ColorRange
{
	Undefined = 0,
	HDR,
	LDR
};

enum class ColorSpace
{
	Undefined = 0,
	Linear,
	GammaCorrected
};

enum class ColorChannel
{
	Undefined = 0,
	R,
	RG,
	RGB,
	RGBA
};

enum class Type
{
	Undefined = 0,
	Texture2D,
	CubeMap,
	PMREM
};

enum class Compression
{
	NoCompress = 0,
	DXT											// auto choose dxt1 for rgb / dxt5 for rgba
};

// -------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)
struct TextureHeader
{
	Type										textureType;
	ColorRange									colorRange;
	ColorSpace									colorSpace;
	ColorChannel								colorChannel;
	f32											encodedGamma;
	u32											mipsCount;
	u32											resolution;
	Compression									compression;
};
#pragma pack(pop)

// ----------------------------------------------------------------------------

template <class TIOAllocator>
const insigne::texture_handle_t					LoadCBTexture(const floral::path& i_path, insigne::texture_desc_t& io_desc, TIOAllocator* i_ioAllocator, const bool i_loadMipmaps = false);

template <class TFileSystem, class TIOAllocator>
const insigne::texture_handle_t					LoadCBTexture(TFileSystem* i_fs, const floral::relative_path& i_path, insigne::texture_desc_t& io_desc, TIOAllocator* i_ioAllocator, const bool i_loadMipmaps = false);

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
