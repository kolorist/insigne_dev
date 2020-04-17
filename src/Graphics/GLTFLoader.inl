#include <stddef.h>

namespace gltf_loader
{
// ----------------------------------------------------------------------------

template <class TMemoryArena>
GLTFLoadResult CreateModel(const_cstr i_jsonScene, const_cstr i_currDir, voidptr o_vtxData, voidptr o_idxData, const size i_stride, const floral::geo_vertex_format_e i_readFlags, TMemoryArena* i_memoryArena)
{
	internal::InitializeJSONParser(i_memoryArena);
	cJSON* json = cJSON_Parse(i_jsonScene);
	FLORAL_ASSERT_MSG(json != nullptr, "Failed to parsed GLTF JSON");

	const cJSON* nodesArray = cJSON_GetObjectItemCaseSensitive(json, "nodes");
	FLORAL_ASSERT_MSG(cJSON_GetArraySize(nodesArray) == 1, "This function can only load 1 node scene");

	const cJSON* meshArray = cJSON_GetObjectItemCaseSensitive(json, "meshes");
	FLORAL_ASSERT_MSG(cJSON_GetArraySize(meshArray) == 1, "This function can only load 1 mesh scene");

	const cJSON* mesh = cJSON_GetArrayItem(meshArray, 0);
	const cJSON* primitives = cJSON_GetObjectItemCaseSensitive(mesh, "primitives");
	const cJSON* primitive = cJSON_GetArrayItem(primitives, 0);
	const cJSON* attributes = cJSON_GetObjectItemCaseSensitive(primitive, "attributes");

	const cJSON* buffers = cJSON_GetObjectItemCaseSensitive(json, "buffers");
	const cJSON* buffer = cJSON_GetArrayItem(buffers, 0);
	const char* uri = cJSON_GetObjectItemCaseSensitive(buffer, "uri")->valuestring;

	// now read it!
	cstr fullBinPath = (cstr)i_memoryArena->allocate(1024);
	sprintf(fullBinPath, "%s/%s", i_currDir, uri);
	floral::file_info inp = floral::open_file(fullBinPath);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)i_memoryArena->allocate(inp.file_size + 1);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);

	GLTFLoadResult loadResult;

	aptr offset = 0;
	const cJSON* buffPosition = cJSON_GetObjectItemCaseSensitive(attributes, "POSITION");
	loadResult.verticesCount = internal::ReadBuffer(buffPosition->valueint, json, (p8)((aptr)o_vtxData + offset), sizeof(floral::vec3f), i_stride, inpStream, i_memoryArena);
	offset += sizeof(floral::vec3f);

	if (TEST_BIT((s32)i_readFlags, (s32)floral::geo_vertex_format_e::normal))
	{
		const cJSON* buffNormal = cJSON_GetObjectItemCaseSensitive(attributes, "NORMAL");
		internal::ReadBuffer(buffNormal->valueint, json, (p8)((aptr)o_vtxData + offset), sizeof(floral::vec3f), i_stride, inpStream, i_memoryArena);
		offset += sizeof(floral::vec3f);
	}

	if (TEST_BIT((s32)i_readFlags, (s32)floral::geo_vertex_format_e::tangent))
	{
		const cJSON* buffTangent = cJSON_GetObjectItemCaseSensitive(attributes, "TANGENT");
		internal::ReadBuffer(buffTangent->valueint, json, (p8)((aptr)o_vtxData + offset), sizeof(floral::vec3f), i_stride, inpStream, i_memoryArena);
		offset += sizeof(floral::vec3f);
	}

	if (TEST_BIT((s32)i_readFlags, (s32)floral::geo_vertex_format_e::tex_coord))
	{
		const cJSON* buffTexCoord = cJSON_GetObjectItemCaseSensitive(attributes, "TEXCOORD_0");
		internal::ReadBuffer(buffTexCoord->valueint, json, (p8)((aptr)o_vtxData + offset), sizeof(floral::vec2f), i_stride, inpStream, i_memoryArena);
		offset += sizeof(floral::vec2f);
	}

	const cJSON* buffIndices = cJSON_GetObjectItemCaseSensitive(primitive, "indices");
	loadResult.indicesCount = internal::ReadBuffer(buffIndices->valueint, json, (p8)(o_idxData), sizeof(s32), sizeof(s32), inpStream, i_memoryArena);
	return loadResult;
}

template <class TMemoryArena>
GLTFLoadResult CreateModel(const floral::path& i_path, voidptr o_vtxData, voidptr o_idxData, const size i_stride, const floral::geo_vertex_format_e i_readFlags, TMemoryArena* i_memoryArena)
{
	floral::file_info inp = floral::open_file(i_path);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)i_memoryArena->allocate(inp.file_size + 1);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);

	return CreateModel((const_cstr)inpStream.buffer, i_path.pm_CurrentDir, o_vtxData, o_idxData, i_stride, i_readFlags, i_memoryArena);
}

// ----------------------------------------------------------------------------

namespace internal
{
// ----------------------------------------------------------------------------

template <class TMemoryArena>
struct AllocatorRegistry
{
	static TMemoryArena* allocator;
	static void* Allocate(size_t sz)
	{
		return allocator->allocate(sz);
	}

	static void Free(void* ptr)
	{
		if (ptr)
		{
			return allocator->free(ptr);
		}
	}
};

template <class TMemoryArena>
TMemoryArena* AllocatorRegistry<TMemoryArena>::allocator = nullptr;

template <class TMemoryArena>
void InitializeJSONParser(TMemoryArena* i_memoryArena)
{
	AllocatorRegistry<TMemoryArena>::allocator = i_memoryArena;
	cJSON_Hooks hooks;
	hooks.malloc_fn = &AllocatorRegistry<TMemoryArena>::Allocate;
	hooks.free_fn = &AllocatorRegistry<TMemoryArena>::Free;
	cJSON_InitHooks(&hooks);
}

template <class TMemoryArena>
const size ReadBuffer(const s32 i_idx, cJSON* i_json, p8 o_buffer, const size i_elemSize, const size i_stride, floral::file_stream& i_inpStream, TMemoryArena* i_memoryArena)
{
	const cJSON* accessors = cJSON_GetObjectItemCaseSensitive(i_json, "accessors");
	const cJSON* accessor = cJSON_GetArrayItem(accessors, i_idx);
	size buffIdx = cJSON_GetObjectItemCaseSensitive(accessor, "bufferView")->valueint;
	size count = cJSON_GetObjectItemCaseSensitive(accessor, "count")->valueint;

	size diskElemSize = 0;
	if (strcmp(cJSON_GetObjectItemCaseSensitive(accessor, "type")->valuestring, "SCALAR") == 0)
	{
		diskElemSize = 4;
	}
	else if (strcmp(cJSON_GetObjectItemCaseSensitive(accessor, "type")->valuestring, "VEC2") == 0)
	{
		diskElemSize = 4 * 2;
	}
	else if (strcmp(cJSON_GetObjectItemCaseSensitive(accessor, "type")->valuestring, "VEC3") == 0)
	{
		diskElemSize = 4 * 3;
	}
	else if (strcmp(cJSON_GetObjectItemCaseSensitive(accessor, "type")->valuestring, "VEC4") == 0)
	{
		diskElemSize = 4 * 4;
	}
	FLORAL_ASSERT(diskElemSize >= i_elemSize);

	const cJSON* bufferViews = cJSON_GetObjectItemCaseSensitive(i_json, "bufferViews");
	const cJSON* bufferView = cJSON_GetArrayItem(bufferViews, buffIdx);
	const size offset = cJSON_GetObjectItemCaseSensitive(bufferView, "byteOffset")->valueint;

	i_inpStream.seek_begin(offset);
	aptr wpos = 0;
	for (size i = 0; i < count; i++)
	{
		i_inpStream.read_bytes(voidptr((aptr)o_buffer + wpos), i_elemSize);
		wpos += i_stride;
	}
	return count;
}

// ----------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
}
