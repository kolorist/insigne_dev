#include "ModelManager.h"

#include "SurfaceDefinitions.h"

namespace stone {
ModelManager::ModelManager(IMaterialManager* i_materialManager)
	: m_MaterialManager(i_materialManager)
{
	m_MemoryArena = g_PersistanceAllocator.allocate_arena<LinearArena>(SIZE_MB(4));
}

ModelManager::~ModelManager()
{
}

void ModelManager::Initialize()
{
}

inline void ReadString(floral::file_stream& i_fileStream, cstr o_str, const u32 i_maxLen)
{
	u32 len = 0;
	memset(o_str, 0, i_maxLen * sizeof(c8));
	i_fileStream.read<u32>(&len);
	i_fileStream.read_bytes(o_str, len);
}

Model* ModelManager::CreateModel(const floral::path& i_path)
{
	typedef floral::fixed_array<Vertex, LinearArena>	VertexArray;
	typedef floral::fixed_array<u32, LinearArena>		IndexArray;
	//TODO: cache

	floral::file_info modelFile = floral::open_file(i_path);
	floral::file_stream dataStream;
	dataStream.buffer = (p8)m_MemoryArena->allocate(modelFile.file_size);
	floral::read_all_file(modelFile, dataStream);
	floral::close_file(modelFile);

	Model* newModel = g_SceneResourceAllocator.allocate<Model>();

#pragma pack(push)
#pragma pack(1)
	struct Header1 {
		c8									magicCharacters[4];
		u32									totalPrimNodes;
		u32									totalMeshNodes;
	};
#pragma pack(pop)

	Header1 h1;
	dataStream.read<Header1>(&h1);

	newModel->surfacesList.init(h1.totalMeshNodes, &g_SceneResourceAllocator);

	c8 rootGeometryName[128];
	ReadString(dataStream, rootGeometryName, 128);

	u32 groupNum = 0;
	dataStream.read<u32>(&groupNum);

	for (u32 i = 0; i < groupNum; i++) {
		c8 groupName[128];
		ReadString(dataStream, groupName, 128);

		u32 groupOffset = 0;
		dataStream.read<u32>(&groupOffset);

		u32 meshNum = 0;
		dataStream.read<u32>(&meshNum);

		for (u32 j = 0; j < meshNum; j++) {
			Surface newSurface;

			u32 meshOffset = 0;
			dataStream.read<u32>(&meshOffset);
			u32 dummyZero = 0;
			dataStream.read<u32>(&dummyZero);
			size homingOffset = dataStream.rpos;
			dataStream.seek_begin(meshOffset);

			c8 materialName[128];
			ReadString(dataStream, materialName, 128);
			// material loading here
			c8 materialPath[512];
			sprintf(materialPath, "gfx/mat/%s.mat", materialName);
			newSurface.materialHdl = m_MaterialManager->CreateMaterialFromFile(floral::path(materialPath));

			u32 verticesCount = 0, indicesCount = 0, vertexFormat = 0;
			dataStream.read<u32>(&vertexFormat);

			dataStream.read<u32>(&verticesCount);
			VertexArray vertices;
			vertices.init(verticesCount, m_MemoryArena);
			for (u32 i = 0; i < verticesCount; i++) {
				Vertex v;
				dataStream.read<Vertex>(&v);
				vertices.push_back(v);
			}

			dataStream.read<u32>(&indicesCount);
			IndexArray indices;
			indices.init(indicesCount, m_MemoryArena);
			for (u32 i = 0; i < indicesCount; i++) {
				u32 idx = 0;
				dataStream.read<u32>(&idx);
				indices.push_back(idx);
			}

			newSurface.surfaceHdl = insigne::upload_surface(&vertices[0], sizeof(Vertex) * vertices.get_size(),
					&indices[0], sizeof(u32) * indices.get_size(),
					sizeof(Vertex), vertices.get_size(), indices.get_size());

			dataStream.seek_begin(homingOffset);

			newModel->surfacesList.push_back(newSurface);
		}
	}

	m_MemoryArena->free_all();

	return newModel;
}

insigne::surface_handle_t ModelManager::CreateSingleSurface(const_cstr i_surfPath)
{
	typedef floral::fixed_array<Vertex, LinearArena>	VertexArray;
	typedef floral::fixed_array<u32, LinearArena>		IndexArray;

	//TODO: cache

	floral::file_info modelFile = floral::open_file(i_surfPath);
	floral::file_stream dataStream;
	dataStream.buffer = (p8)m_MemoryArena->allocate(modelFile.file_size);
	floral::read_all_file(modelFile, dataStream);

	c8 magicChars[4];
	dataStream.read_bytes(magicChars, 4);

	c8 rootGeoName[128];
	ReadString(dataStream, rootGeoName, 128);

	u32 groupNum = 0;
	dataStream.read<u32>(&groupNum);

	c8 groupName[128];
	ReadString(dataStream, groupName, 128);

	u32 groupOffset = 0;
	dataStream.read<u32>(&groupOffset);
	u32 meshNum = 0;
	dataStream.read<u32>(&meshNum);

	u32 meshOffset = 0;
	dataStream.read<u32>(&meshOffset);
	u32 dummyZero = 0;
	dataStream.read<u32>(&dummyZero);
	dataStream.seek_begin(meshOffset);

	c8 meshName[128];
	ReadString(dataStream, meshName, 128);

	u32 verticesCount = 0, indicesCount = 0, vertexFormat = 0;
	dataStream.read<u32>(&vertexFormat);

	dataStream.read<u32>(&verticesCount);
	VertexArray* vertices = m_MemoryArena->allocate<VertexArray>(verticesCount, m_MemoryArena);
	for (u32 i = 0; i < verticesCount; i++) {
		Vertex v;
		dataStream.read<Vertex>(&v);
		vertices->push_back(v);
	}

	dataStream.read<u32>(&indicesCount);
	IndexArray* indices = m_MemoryArena->allocate<IndexArray>(indicesCount, m_MemoryArena);
	for (u32 i = 0; i < indicesCount; i++) {
		u32 idx = 0;
		dataStream.read<u32>(&idx);
		indices->push_back(idx);
	}

	insigne::surface_handle_t shdl = insigne::upload_surface(&(*vertices)[0], sizeof(Vertex) * vertices->get_size(),
			&(*indices)[0], sizeof(u32) * indices->get_size(),
			sizeof(Vertex), vertices->get_size(), indices->get_size());

	m_MemoryArena->free_all();
	floral::close_file(modelFile);
	return shdl;
}
}
