#pragma once

#include <floral/stdaliases.h>

namespace cb
{

/* CBOBJ format
 * ---------------------------------------------
 * [ModelDataHeader]
 * [LOD_0 Info]
 * [LOD_1 Info]
 * <...>
 * [LOD_n Info]
 * <...>
 * [Position Data Segment]
 * [Normal Data Segment]
 * [Binormal Data Segment]
 * [TexCoord Data Segment]
 * [VertexColor Data Segment]
 */

#define MAX_LODS								4
#define NORMAL_CHANNELS_COUNT					2
#define BINORMAL_CHANNELS_COUNT					2
#define TANGENT_CHANNELS_COUNT					2
#define TEXCOORD_CHANNELS_COUNT					2
#define VTXCOLOR_CHANNELS_COUNT					2

struct ModelDataHeader
{
	u32											Version;

	// LOD infos
	u32											LODsCount;

	// offset to data segments (in bytes)
	u64											PositionOffset;								// floral::vec3f
	u64											IndexOffset;								// u32
	u64											NormalOffsets[NORMAL_CHANNELS_COUNT];		// floral::vec3f
	u64											BinormalOffsets[BINORMAL_CHANNELS_COUNT];	// floral::vec3f
	u64											TangentOffsets[TANGENT_CHANNELS_COUNT];		// floral::vec3f
	u64											TexCoordOffsets[TEXCOORD_CHANNELS_COUNT];	// floral::vec2f
	u64											VtxColorOffsets[VTXCOLOR_CHANNELS_COUNT];	// floral::vec4f
};

struct ModelLODInfo
{
	u32											LODIndex;
	u32											VertexCount;
	u32											IndexCount;

	// offsets (in elements count)
	u32											OffsetToVertexData;
	u32											OffsetToIndexData;
};

}
