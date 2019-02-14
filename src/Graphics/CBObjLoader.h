#pragma once

#include <stdaliases.h>
#include <io/nativeio.h>
#include <cmds/path.h>
#include <containers/array.h>

#include "CBObjDefinitions.h"

namespace cb
{

template <class TAllocator>
class ModelLoader
{
	public:
		ModelLoader(TAllocator* i_allocator);
		~ModelLoader();

		void									LoadFromFile(const floral::path& i_path);

		const u32								GetVerticesCount(const u32 i_lodIndex);
		const u32								GetIndicesCount(const u32 i_lodIndex);

		template <class TArrayAllocator>
		void									ExtractPositionData(const u32 i_lodIndex, floral::fixed_array<floral::vec3f, TArrayAllocator>& o_posArray);
		void									ExtractPositionData(const u32 i_lodIndex, const size i_stride, const size i_offset, voidptr o_target);
		void									ExtractIndexData(const u32 i_lodIndex, const size i_stride, const size i_offset, voidptr o_target);

	private:
		floral::file_info						m_ModelFile;
		floral::file_stream						m_DataStream;

		ModelDataHeader							m_Header;
		ModelLODInfo							m_LODInfos[MAX_LODS];

		TAllocator*								m_Allocator;
};

}

#include "CBObjLoader.inl"
