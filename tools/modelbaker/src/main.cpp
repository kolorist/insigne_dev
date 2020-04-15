#include <floral/stdaliases.h>
#include <floral/io/nativeio.h>

#include <helich.h>
#include <clover.h>
#include <stdio.h>

#include "Memory/MemorySystem.h"

#include "cJSON.h"

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

struct BufferViewDescription
{
	s32											componentType;
	s32											elementCount;
	const_cstr									elementType;

	size										byteOffset;
	size										byteLength;
};

BufferViewDescription GetBufferViewDescription(cJSON* i_accessors, cJSON* i_bufferViews, const size i_accessorIdx)
{
	BufferViewDescription desc;

	cJSON* accessor = cJSON_GetArrayItem(i_accessors, i_accessorIdx);
	size bufferViewIdx = cJSON_GetObjectItemCaseSensitive(accessor, "bufferView")->valueint;
	desc.componentType = cJSON_GetObjectItemCaseSensitive(accessor, "componentType")->valueint;
	desc.elementCount = cJSON_GetObjectItemCaseSensitive(accessor, "count")->valueint;
	desc.elementType = cJSON_GetObjectItemCaseSensitive(accessor, "type")->valuestring;

	cJSON* bufferView = cJSON_GetArrayItem(i_bufferViews, bufferViewIdx);
	desc.byteOffset = cJSON_GetObjectItemCaseSensitive(bufferView, "byteOffset")->valueint;
	desc.byteLength = cJSON_GetObjectItemCaseSensitive(bufferView, "byteLength")->valueint;

	return desc;
}

int main(int argc, char** argv)
{
	using namespace baker;
	// we have to call it ourself as we do not have calyx here
	helich::init_memory_system();

	clover::Initialize("main", clover::LogLevel::Verbose);
	clover::InitializeVSOutput("vs", clover::LogLevel::Verbose);
	clover::InitializeConsoleOutput("console", clover::LogLevel::Verbose);

	CLOVER_INFO("Model Baker v3");

	floral::file_info inp = floral::open_file(argv[1]);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)g_TemporalArena.allocate(inp.file_size + 1);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);

	size selectedNode = 0;

	InitializeJSONParser(&g_ParserAllocator);
	cJSON* json = cJSON_Parse((const_cstr)inpStream.buffer);

	cJSON* nodes = cJSON_GetObjectItemCaseSensitive(json, "nodes");
	CLOVER_INFO("Nodes count: %d", cJSON_GetArraySize(nodes));

	cJSON* meshes = cJSON_GetObjectItemCaseSensitive(json, "meshes");
	CLOVER_INFO("Meshes count: %d", cJSON_GetArraySize(meshes));

	cJSON* accessors = cJSON_GetObjectItemCaseSensitive(json, "accessors");
	CLOVER_INFO("Accessors count: %d", cJSON_GetArraySize(accessors));

	cJSON* bufferViews = cJSON_GetObjectItemCaseSensitive(json, "bufferViews");
	CLOVER_INFO("BufferViews count: %d", cJSON_GetArraySize(bufferViews));

	cJSON* buffers = cJSON_GetObjectItemCaseSensitive(json, "buffers");
	CLOVER_INFO("Buffers count: %d", cJSON_GetArraySize(buffers));

	// read buffer into memory here
	cJSON* buffer = cJSON_GetArrayItem(buffers, 0);
	CLOVER_INFO("Buffer file name: %s", cJSON_GetObjectItemCaseSensitive(buffer, "uri")->valuestring);
	CLOVER_INFO("Buffer file size: %d", cJSON_GetObjectItemCaseSensitive(buffer, "byteLength")->valueint);

	CLOVER_INFO(">>> Begin exporting");
	CLOVER_INFO("Exporting node #%d", selectedNode);
	cJSON* node = cJSON_GetArrayItem(nodes, selectedNode);
	CLOVER_INFO("- name: %s", cJSON_GetObjectItemCaseSensitive(node, "name")->valuestring);
	size meshIdx = cJSON_GetObjectItemCaseSensitive(node, "mesh")->valueint;
	CLOVER_INFO("- mesh: %d", meshIdx);

	cJSON* mesh = cJSON_GetArrayItem(meshes, meshIdx);
	cJSON* primitives = cJSON_GetObjectItemCaseSensitive(mesh, "primitives");
	size primitivesCount = cJSON_GetArraySize(primitives);
	CLOVER_INFO("-- primitives count: %d", primitivesCount);
	for (size i = 0; i < primitivesCount; i++)
	{
		cJSON* primitive = cJSON_GetArrayItem(primitives, i);
		CLOVER_INFO("--- primitive #%d", i);

		{
			size accessorIdx = cJSON_GetObjectItemCaseSensitive(primitive, "indices")->valueint;
			CLOVER_INFO("---- indices accessor index: %d", accessorIdx);

			BufferViewDescription desc = GetBufferViewDescription(accessors, bufferViews, accessorIdx);

			CLOVER_INFO("------ componentType %d", desc.componentType);
			CLOVER_INFO("------ elementCount %d", desc.elementCount);
			CLOVER_INFO("------ elementType %s", desc.elementType);

			CLOVER_INFO("------ from offset byte: %d", desc.byteOffset);
			CLOVER_INFO("------ length: %d bytes", desc.byteLength);
		}

		cJSON* attributes = cJSON_GetObjectItemCaseSensitive(primitive, "attributes");
		cJSON* position = cJSON_GetObjectItemCaseSensitive(attributes, "POSITION");
		if (position)
		{
			CLOVER_INFO("---- POSITION accessor index: %d", position->valueint);
			BufferViewDescription desc = GetBufferViewDescription(accessors, bufferViews, position->valueint);

			CLOVER_INFO("------ componentType %d", desc.componentType);
			CLOVER_INFO("------ elementCount %d", desc.elementCount);
			CLOVER_INFO("------ elementType %s", desc.elementType);

			CLOVER_INFO("------ from offset byte: %d", desc.byteOffset);
			CLOVER_INFO("------ length: %d bytes", desc.byteLength);
		}

		cJSON* normal = cJSON_GetObjectItemCaseSensitive(attributes, "NORMAL");
		if (normal)
		{
			CLOVER_INFO("---- NORMAL accessor index: %d", normal->valueint);
			BufferViewDescription desc = GetBufferViewDescription(accessors, bufferViews, normal->valueint);

			CLOVER_INFO("------ componentType %d", desc.componentType);
			CLOVER_INFO("------ elementCount %d", desc.elementCount);
			CLOVER_INFO("------ elementType %s", desc.elementType);

			CLOVER_INFO("------ from offset byte: %d", desc.byteOffset);
			CLOVER_INFO("------ length: %d bytes", desc.byteLength);
		}

		cJSON* tangent = cJSON_GetObjectItemCaseSensitive(attributes, "TANGENT");
		if (tangent)
		{
			CLOVER_INFO("---- TANGENT accessor index: %d", tangent->valueint);
			BufferViewDescription desc = GetBufferViewDescription(accessors, bufferViews, tangent->valueint);

			CLOVER_INFO("------ componentType %d", desc.componentType);
			CLOVER_INFO("------ elementCount %d", desc.elementCount);
			CLOVER_INFO("------ elementType %s", desc.elementType);

			CLOVER_INFO("------ from offset byte: %d", desc.byteOffset);
			CLOVER_INFO("------ length: %d bytes", desc.byteLength);
		}

		cJSON* texcoord = cJSON_GetObjectItemCaseSensitive(attributes, "TEXCOORD_0");
		if (texcoord)
		{
			CLOVER_INFO("---- TEXCOORD_0 accessor index: %d", texcoord->valueint);
			BufferViewDescription desc = GetBufferViewDescription(accessors, bufferViews, texcoord->valueint);

			CLOVER_INFO("------ componentType %d", desc.componentType);
			CLOVER_INFO("------ elementCount %d", desc.elementCount);
			CLOVER_INFO("------ elementType %s", desc.elementType);

			CLOVER_INFO("------ from offset byte: %d", desc.byteOffset);
			CLOVER_INFO("------ length: %d bytes", desc.byteLength);
		}
	}

	CLOVER_INFO("<<< End exporting");

	return 0;
}
