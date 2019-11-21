#pragma once

#include <floral/stdaliases.h>
#include <floral/io/nativeio.h>
#include <floral/cmds/path.h>
#include <floral/containers/array.h>

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

		template <class TArrayAllocator>
		void									ExtractPositionData(const u32 i_lodIndex, floral::fixed_array<floral::vec3f, TArrayAllocator>& o_posArray);

	private:
		floral::file_info						m_ModelFile;
		floral::file_stream						m_DataStream;

		ModelDataHeader							m_Header;
		ModelLODInfo							m_LODInfos[MAX_LODS];

		TAllocator*								m_Allocator;
};

}

#include "CBObjLoader.inl"
