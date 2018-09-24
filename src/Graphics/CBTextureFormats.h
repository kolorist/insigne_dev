#pragma once

#include <floral.h>

namespace cymbi {

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
};

#pragma pack(pop)

// ---------------------------------------------
#pragma pack(push)
#pragma pack(1)

struct CBSHHeader {
	c8											magicCharacters[4];		// "CBSH"
	u32											numCoeffs;
	u32											numSHs;
};

#pragma pack(pop)

}
