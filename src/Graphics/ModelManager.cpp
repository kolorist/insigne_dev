#include "ModelManager.h"

namespace stone {
	ModelManager::ModelManager()
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

	insigne::surface_handle_t ModelManager::CreateSingleSurface(const_cstr i_surfPath)
	{
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

		m_MemoryArena->free_all();
		floral::close_file(modelFile);
		return 0;
	}
}
