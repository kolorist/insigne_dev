#include <floral/stdaliases.h>
#include <floral/io/nativeio.h>
#include <floral/gpds/vec.h>
#include <floral/gpds/quaternion.h>

#include <helich.h>
#include <clover.h>
#include <stdio.h>

#include "Memory/MemorySystem.h"

#include "cJSON.h"

#pragma pack(push)
#pragma pack(1)
struct CbModelHeader
{
	s32											indicesCount;
	s32											verticesCount;

	size										indicesOffset;
	size										positionOffset;
	size										normalOffset;
	size										tangentOffset;
	size										texcoordOffset;
};

struct CbNode
{
	floral::vec3f								translation;
	floral::quaternionf							rotation;
	floral::vec3f								scale;
};

struct CbSceneHeader
{
	size										nodesCount;
};
#pragma pack(pop)

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

enum class ComponentType : s32
{
	Invalid = 0,
	Byte = 5120,
	UnsignedByte = 5121,
	Short = 5122,
	UnsignedShort = 5123,
	UnsignedInt = 5125,
	Float = 5126
};

enum class ElementType : s32
{
	Invalid = 0,
	Scalar,
	Vec2,
	Vec3,
	Vec4,
	Mat2,
	Mat3,
	Mat4
};

struct BufferViewDescription
{
	ComponentType								componentType;
	ElementType									elementType;
	s32											elementCount;

	size										byteOffset;
	size										byteLength;
};

inline const ComponentType GetComponentTypeFromValue(const s32 i_componentTypeValue)
{
	return ComponentType(i_componentTypeValue);
}

inline const ElementType GetElementTypeFromValue(const_cstr i_elementTypeValue)
{
	if (strcmp(i_elementTypeValue, "SCALAR") == 0)
	{
		return ElementType::Scalar;
	}
	else if (strcmp(i_elementTypeValue, "VEC2") == 0)
	{
		return ElementType::Vec2;
	}
	else if (strcmp(i_elementTypeValue, "VEC3") == 0)
	{
		return ElementType::Vec3;
	}
	else if (strcmp(i_elementTypeValue, "VEC4") == 0)
	{
		return ElementType::Vec4;
	}
	else if (strcmp(i_elementTypeValue, "MAT2") == 0)
	{
		return ElementType::Mat2;
	}
	else if (strcmp(i_elementTypeValue, "MAT3") == 0)
	{
		return ElementType::Mat3;
	}
	else if (strcmp(i_elementTypeValue, "MAT4") == 0)
	{
		return ElementType::Mat4;
	}

	return ElementType::Invalid;
}

BufferViewDescription GetBufferViewDescription(cJSON* i_accessors, cJSON* i_bufferViews, const size i_accessorIdx)
{
	BufferViewDescription desc;

	cJSON* accessor = cJSON_GetArrayItem(i_accessors, i_accessorIdx);
	size bufferViewIdx = cJSON_GetObjectItemCaseSensitive(accessor, "bufferView")->valueint;
	desc.componentType = GetComponentTypeFromValue(cJSON_GetObjectItemCaseSensitive(accessor, "componentType")->valueint);
	desc.elementType = GetElementTypeFromValue(cJSON_GetObjectItemCaseSensitive(accessor, "type")->valuestring);
	desc.elementCount = cJSON_GetObjectItemCaseSensitive(accessor, "count")->valueint;

	cJSON* bufferView = cJSON_GetArrayItem(i_bufferViews, bufferViewIdx);
	desc.byteOffset = cJSON_GetObjectItemCaseSensitive(bufferView, "byteOffset")->valueint;
	desc.byteLength = cJSON_GetObjectItemCaseSensitive(bufferView, "byteLength")->valueint;

	return desc;
}

void ExportFirstNodeAsModel(const_cstr i_gltfFile, const_cstr i_outputFile)
{
	using namespace baker;
	floral::file_info inp = floral::open_file(i_gltfFile);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)g_ParserAllocator.allocate(inp.file_size + 1);
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
	const_cstr binFilename = cJSON_GetObjectItemCaseSensitive(buffer, "uri")->valuestring;
	CLOVER_INFO("Buffer file name: %s", binFilename);
	CLOVER_INFO("Buffer file size: %d", cJSON_GetObjectItemCaseSensitive(buffer, "byteLength")->valueint);
	floral::file_info binInp = floral::open_file(binFilename);
	floral::file_stream binInpStream;
	binInpStream.buffer = (p8)g_TemporalAllocator.allocate(binInp.file_size);
	floral::read_all_file(binInp, binInpStream);
	floral::close_file(binInp);

	CLOVER_INFO(">>> Begin exporting");
	CLOVER_INFO("Exporting node #%d", selectedNode);
	cJSON* node = cJSON_GetArrayItem(nodes, selectedNode);
	CLOVER_INFO("- name: %s", cJSON_GetObjectItemCaseSensitive(node, "name")->valuestring);
	size meshIdx = cJSON_GetObjectItemCaseSensitive(node, "mesh")->valueint;
	CLOVER_INFO("- mesh: %d", meshIdx);

	floral::file_info output = floral::open_output_file(i_outputFile);
	floral::output_file_stream os;
	floral::map_output_file(output, os);

	CbModelHeader header;
	os.write(header);

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
			binInpStream.seek_begin(desc.byteOffset);
			header.indicesCount = desc.elementCount;
			header.indicesOffset = os.get_pointer_position();
			for (size i = 0; i < desc.elementCount; i++)
			{
				s32 cbIndex = -1;
				if (desc.componentType == ComponentType::UnsignedShort)
				{
					u16 gltfIndex = 0;
					binInpStream.read(&gltfIndex);
					cbIndex = (s32)gltfIndex;
				}
				else if (desc.componentType == ComponentType::UnsignedInt)
				{
					u32 gltfIndex = 0;
					binInpStream.read(&gltfIndex);
					cbIndex = (s32)gltfIndex;
				}
				os.write(cbIndex);
			}
		}

		cJSON* attributes = cJSON_GetObjectItemCaseSensitive(primitive, "attributes");
		cJSON* position = cJSON_GetObjectItemCaseSensitive(attributes, "POSITION");
		if (position)
		{
			CLOVER_INFO("---- POSITION accessor index: %d", position->valueint);
			BufferViewDescription desc = GetBufferViewDescription(accessors, bufferViews, position->valueint);
			header.verticesCount = desc.elementCount;
			header.positionOffset = os.get_pointer_position();
			binInpStream.seek_begin(desc.byteOffset);
			for (size i = 0; i < desc.elementCount; i++)
			{
				floral::vec3f v(0.0f, 0.0f, 0.0f);
				binInpStream.read(&v);
				os.write(v);
			}
		}

		cJSON* normal = cJSON_GetObjectItemCaseSensitive(attributes, "NORMAL");
		if (normal)
		{
			CLOVER_INFO("---- NORMAL accessor index: %d", normal->valueint);
			BufferViewDescription desc = GetBufferViewDescription(accessors, bufferViews, normal->valueint);
			header.normalOffset = os.get_pointer_position();
			binInpStream.seek_begin(desc.byteOffset);
			for (size i = 0; i < desc.elementCount; i++)
			{
				floral::vec3f v(0.0f, 0.0f, 0.0f);
				binInpStream.read(&v);
				os.write(v);
			}
		}

		cJSON* tangent = cJSON_GetObjectItemCaseSensitive(attributes, "TANGENT");
		if (tangent)
		{
			CLOVER_INFO("---- TANGENT accessor index: %d", tangent->valueint);
			BufferViewDescription desc = GetBufferViewDescription(accessors, bufferViews, tangent->valueint);
			header.tangentOffset = os.get_pointer_position();
			binInpStream.seek_begin(desc.byteOffset);
			for (size i = 0; i < desc.elementCount; i++)
			{
				floral::vec4f v(0.0f, 0.0f, 0.0f, 0.0f);
				binInpStream.read(&v);
				floral::vec3f cbVec(v.x, v.y, v.z);
				os.write(cbVec);
			}
		}

		cJSON* texcoord = cJSON_GetObjectItemCaseSensitive(attributes, "TEXCOORD_0");
		if (texcoord)
		{
			CLOVER_INFO("---- TEXCOORD_0 accessor index: %d", texcoord->valueint);
			BufferViewDescription desc = GetBufferViewDescription(accessors, bufferViews, texcoord->valueint);
			header.texcoordOffset = os.get_pointer_position();
			binInpStream.seek_begin(desc.byteOffset);
			for (size i = 0; i < desc.elementCount; i++)
			{
				floral::vec2f v(0.0f, 0.0f);
				binInpStream.read(&v);
				os.write(v);
			}
		}
	}

	os.seek_begin(0);
	os.write(header);

	floral::close_file(output);

	CLOVER_INFO("<<< End exporting");
}

void ExportMeshNoOverwrite(cJSON* i_mesh, cJSON* i_materials, cJSON* i_accessors, cJSON* i_bufferViews, floral::file_stream& i_binStream, const_cstr i_outputFileName)
{
	floral::file_info output = floral::open_output_file(i_outputFileName);
	floral::output_file_stream os;
	floral::map_output_file(output, os);

	CbModelHeader header;
	os.write(header);

	cJSON* primitives = cJSON_GetObjectItemCaseSensitive(i_mesh, "primitives");
	size primitivesCount = cJSON_GetArraySize(primitives);
	CLOVER_INFO("-- primitives count: %d", primitivesCount);
	for (size i = 0; i < primitivesCount; i++)
	{
		cJSON* primitive = cJSON_GetArrayItem(primitives, i);
		CLOVER_INFO("--- primitive #%d", i);

		// material
		{
			size materialIdx = cJSON_GetObjectItemCaseSensitive(primitive, "material")->valueint;
			cJSON* material = cJSON_GetArrayItem(i_materials, materialIdx);
			const_cstr materialName = cJSON_GetObjectItemCaseSensitive(material, "name")->valuestring;
			CLOVER_INFO("---- material name: %s", materialName);
			s32 materialNameLen = strlen(materialName);
			os.write(materialNameLen);
			os.write_bytes((voidptr)materialName, materialNameLen);
		}

		{
			size accessorIdx = cJSON_GetObjectItemCaseSensitive(primitive, "indices")->valueint;
			CLOVER_INFO("---- indices accessor index: %d", accessorIdx);

			BufferViewDescription desc = GetBufferViewDescription(i_accessors, i_bufferViews, accessorIdx);
			i_binStream.seek_begin(desc.byteOffset);
			header.indicesCount = desc.elementCount;
			header.indicesOffset = os.get_pointer_position();
			for (size i = 0; i < desc.elementCount; i++)
			{
				s32 cbIndex = -1;
				if (desc.componentType == ComponentType::UnsignedShort)
				{
					u16 gltfIndex = 0;
					i_binStream.read(&gltfIndex);
					cbIndex = (s32)gltfIndex;
				}
				else if (desc.componentType == ComponentType::UnsignedInt)
				{
					u32 gltfIndex = 0;
					i_binStream.read(&gltfIndex);
					cbIndex = (s32)gltfIndex;
				}
				os.write(cbIndex);
			}
		}

		cJSON* attributes = cJSON_GetObjectItemCaseSensitive(primitive, "attributes");
		cJSON* position = cJSON_GetObjectItemCaseSensitive(attributes, "POSITION");
		if (position)
		{
			CLOVER_INFO("---- POSITION accessor index: %d", position->valueint);
			BufferViewDescription desc = GetBufferViewDescription(i_accessors, i_bufferViews, position->valueint);
			header.verticesCount = desc.elementCount;
			header.positionOffset = os.get_pointer_position();
			i_binStream.seek_begin(desc.byteOffset);
			for (size i = 0; i < desc.elementCount; i++)
			{
				floral::vec3f v(0.0f, 0.0f, 0.0f);
				i_binStream.read(&v);
				os.write(v);
			}
		}

		cJSON* normal = cJSON_GetObjectItemCaseSensitive(attributes, "NORMAL");
		if (normal)
		{
			CLOVER_INFO("---- NORMAL accessor index: %d", normal->valueint);
			BufferViewDescription desc = GetBufferViewDescription(i_accessors, i_bufferViews, normal->valueint);
			header.normalOffset = os.get_pointer_position();
			i_binStream.seek_begin(desc.byteOffset);
			for (size i = 0; i < desc.elementCount; i++)
			{
				floral::vec3f v(0.0f, 0.0f, 0.0f);
				i_binStream.read(&v);
				os.write(v);
			}
		}

		cJSON* tangent = cJSON_GetObjectItemCaseSensitive(attributes, "TANGENT");
		if (tangent)
		{
			CLOVER_INFO("---- TANGENT accessor index: %d", tangent->valueint);
			BufferViewDescription desc = GetBufferViewDescription(i_accessors, i_bufferViews, tangent->valueint);
			header.tangentOffset = os.get_pointer_position();
			i_binStream.seek_begin(desc.byteOffset);
			for (size i = 0; i < desc.elementCount; i++)
			{
				floral::vec4f v(0.0f, 0.0f, 0.0f, 0.0f);
				i_binStream.read(&v);
				floral::vec3f cbVec(v.x, v.y, v.z);
				os.write(cbVec);
			}
		}

		cJSON* texcoord = cJSON_GetObjectItemCaseSensitive(attributes, "TEXCOORD_0");
		if (texcoord)
		{
			CLOVER_INFO("---- TEXCOORD_0 accessor index: %d", texcoord->valueint);
			BufferViewDescription desc = GetBufferViewDescription(i_accessors, i_bufferViews, texcoord->valueint);
			header.texcoordOffset = os.get_pointer_position();
			i_binStream.seek_begin(desc.byteOffset);
			for (size i = 0; i < desc.elementCount; i++)
			{
				floral::vec2f v(0.0f, 0.0f);
				i_binStream.read(&v);
				os.write(v);
			}
		}
	}

	os.seek_begin(0);
	os.write(header);
	floral::close_file(output);
}

void ExportScene(const_cstr i_gltfFile, const_cstr i_outputDir)
{
	using namespace baker;
	floral::file_info inp = floral::open_file(i_gltfFile);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)g_ParserAllocator.allocate(inp.file_size + 1);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);

	InitializeJSONParser(&g_ParserAllocator);
	cJSON* json = cJSON_Parse((const_cstr)inpStream.buffer);

	cJSON* scenes = cJSON_GetObjectItemCaseSensitive(json, "scenes");
	CLOVER_INFO("Scenes count: %d", cJSON_GetArraySize(scenes));

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

	cJSON* materials = cJSON_GetObjectItemCaseSensitive(json, "materials");
	CLOVER_INFO("Materials count: %d", cJSON_GetArraySize(materials));

	// read buffer into memory here
	cJSON* buffer = cJSON_GetArrayItem(buffers, 0);
	const_cstr binFilename = cJSON_GetObjectItemCaseSensitive(buffer, "uri")->valuestring;
	CLOVER_INFO("Buffer file name: %s", binFilename);
	CLOVER_INFO("Buffer file size: %d", cJSON_GetObjectItemCaseSensitive(buffer, "byteLength")->valueint);
	floral::file_info binInp = floral::open_file(binFilename);
	floral::file_stream binInpStream;
	binInpStream.buffer = (p8)g_TemporalAllocator.allocate(binInp.file_size);
	floral::read_all_file(binInp, binInpStream);
	floral::close_file(binInp);

	c8 outputSceneFileName[512];
	sprintf(outputSceneFileName, "%s.cbscene", i_gltfFile);
	floral::file_info output = floral::open_output_file(outputSceneFileName);
	floral::output_file_stream os;
	floral::map_output_file(output, os);

	CLOVER_INFO(">>> Begin exporting");
	s32 selectedScene = cJSON_GetObjectItemCaseSensitive(json, "scene")->valueint;
	CLOVER_INFO("Exporting scene #%d", selectedScene);
	cJSON* scene = cJSON_GetArrayItem(scenes, selectedScene);
	cJSON* selectedNodes = cJSON_GetObjectItemCaseSensitive(scene, "nodes");
	size selectedNodesCount = cJSON_GetArraySize(selectedNodes);

	CbSceneHeader sceneHeader;
	sceneHeader.nodesCount = 0;
	os.write(sceneHeader);

	CLOVER_INFO("Exporting %d nodes", selectedNodesCount);
	for (size i = 0; i < selectedNodesCount; i++)
	{
		size selectedNodeIdx = cJSON_GetArrayItem(selectedNodes, i)->valueint;
		CLOVER_INFO("Exporting node: #%d", selectedNodeIdx);
		cJSON* node = cJSON_GetArrayItem(nodes, selectedNodeIdx);
		const_cstr nodeName = cJSON_GetObjectItemCaseSensitive(node, "name")->valuestring;
		CLOVER_INFO("- name: %s", nodeName);
		if (cJSON_HasObjectItem(node, "mesh"))
		{
			size meshIdx = cJSON_GetObjectItemCaseSensitive(node, "mesh")->valueint;
			CLOVER_INFO("- mesh: %d", meshIdx);
			cJSON* mesh = cJSON_GetArrayItem(meshes, meshIdx);
			const_cstr meshName = cJSON_GetObjectItemCaseSensitive(mesh, "name")->valuestring;
			CLOVER_INFO("-- name: %s", meshName);
			c8 outputModelFileName[512];
			sprintf(outputModelFileName, "%s_%s_%llu.cbmodel", i_gltfFile, meshName, meshIdx);
			CLOVER_INFO("-- output filename: %s", outputModelFileName);
			ExportMeshNoOverwrite(mesh, materials, accessors, bufferViews, binInpStream, outputModelFileName);

			CbNode nodeInfo;
			nodeInfo.translation = floral::vec3f(0.0f, 0.0f, 0.0f);
			nodeInfo.rotation = floral::quaternionf();
			nodeInfo.scale = floral::vec3f(1.0f, 1.0f, 1.0f);

			if (cJSON_HasObjectItem(node, "translation"))
			{
				cJSON* translation = cJSON_GetObjectItemCaseSensitive(node, "translation");
				nodeInfo.translation.x = (f32)(cJSON_GetArrayItem(translation, 0)->valuedouble);
				nodeInfo.translation.y = (f32)(cJSON_GetArrayItem(translation, 1)->valuedouble);
				nodeInfo.translation.z = (f32)(cJSON_GetArrayItem(translation, 2)->valuedouble);
			}

			if (cJSON_HasObjectItem(node, "rotation"))
			{
				cJSON* rotation = cJSON_GetObjectItemCaseSensitive(node, "rotation");
				nodeInfo.rotation.v.x = (f32)(cJSON_GetArrayItem(rotation, 0)->valuedouble);
				nodeInfo.rotation.v.y = (f32)(cJSON_GetArrayItem(rotation, 1)->valuedouble);
				nodeInfo.rotation.v.z = (f32)(cJSON_GetArrayItem(rotation, 2)->valuedouble);
				nodeInfo.rotation.w = (f32)(cJSON_GetArrayItem(rotation, 2)->valuedouble);
			}

			if (cJSON_HasObjectItem(node, "scale"))
			{
				cJSON* scale = cJSON_GetObjectItemCaseSensitive(node, "scale");
				nodeInfo.scale.x = (f32)(cJSON_GetArrayItem(scale, 0)->valuedouble);
				nodeInfo.scale.y = (f32)(cJSON_GetArrayItem(scale, 1)->valuedouble);
				nodeInfo.scale.z = (f32)(cJSON_GetArrayItem(scale, 2)->valuedouble);
			}

			s32 nodeNameLen = strlen(nodeName);
			os.write(nodeNameLen);
			os.write_bytes((voidptr)nodeName, nodeNameLen);
			s32 fileNamelen = strlen(outputModelFileName);
			os.write(fileNamelen);
			os.write_bytes((voidptr)outputModelFileName, fileNamelen);
			os.write(nodeInfo);
			sceneHeader.nodesCount++;
		}
		else
		{
			CLOVER_INFO("non-geometry node...skipping...");
		}
	}
	
	os.seek_begin(0);
	os.write(sceneHeader);
	floral::close_file(output);

	CLOVER_INFO("<<< End exporting");
}

int main(int argc, char** argv)
{
	// we have to call it ourself as we do not have calyx here
	helich::init_memory_system();

	clover::Initialize("main", clover::LogLevel::Verbose);
	clover::InitializeVSOutput("vs", clover::LogLevel::Verbose);
	clover::InitializeConsoleOutput("console", clover::LogLevel::Verbose);

	CLOVER_INFO("Model Baker v3");

	//ExportFirstNodeAsModel(argv[1], argv[2]);
	ExportScene(argv[1], argv[2]);

	return 0;
}
