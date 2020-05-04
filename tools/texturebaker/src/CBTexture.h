#pragma once

#include <floral.h>

namespace cbtex
{
// -------------------------------------------------------------------

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

// -------------------------------------------------------------------
}
