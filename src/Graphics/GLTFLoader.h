#pragma once

#include <floral/stdaliases.h>
#include <floral/cmds/path.h>
#include <floral/comgeo/shapegen.h>
#include <floral/io/nativeio.h>

#include "cJSON.h"

namespace gltf_loader
{
// ----------------------------------------------------------------------------

struct GLTFLoadResult
{
	size										verticesCount;
	size										indicesCount;
};

// These functions only accept gltf files which have only 1 node, this node only contains 1 mesh
template <class TMemoryArena>
GLTFLoadResult									CreateModel(const_cstr i_jsonScene, const_cstr i_currDir, voidptr o_vtxData, voidptr o_idxData, const size i_stride, const floral::geo_vertex_format_e i_readFlags, TMemoryArena* i_memoryArena);
template <class TMemoryArena>
GLTFLoadResult									CreateModel(const floral::path& i_path, voidptr o_vtxData, voidptr o_idxData, const size i_stride, const floral::geo_vertex_format_e i_readFlags, TMemoryArena* i_memoryArena);

namespace internal
{
// ----------------------------------------------------------------------------

template <class TMemoryArena>
void											InitializeJSONParser(TMemoryArena* i_memoryArena);

template <class TMemoryArena>
const size										ReadBuffer(const s32 i_idx, cJSON* i_json, p8 o_buffer, const size i_elemSize, const size i_stride, floral::file_stream& i_inpStream, TMemoryArena* i_memoryArena);

// ----------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
}

#include "GLTFLoader.inl"
