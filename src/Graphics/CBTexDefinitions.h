#pragma once

#include <floral/stdaliases.h>

namespace cb
{

enum class ColorRange {
	Undefined									= 0,
	HDR											= 1,
	LDR											= 2
};

enum class ColorSpace {
	Undefined									= 0,
	Linear										= 1,
	GammaCorrected								= 2
};

enum class ColorChannel {
	Undefined									= 0,
	R											= 1,
	RG											= 2,
	RGB											= 3,
	RGBA										= 4
};

// ---------------------------------------------

#pragma pack(push)
#pragma pack(1)

struct CBTexture2DHeader {
	c8											magicCharacters[4];		// "CBFM"
	ColorRange									colorRange;
	ColorSpace									colorSpace;
	ColorChannel								colorChannel;
	f32											encodedGamma;
	u32											mipsCount;
	u32											resolution;
};

#pragma pack(pop)

}
