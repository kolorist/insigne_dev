#pragma once

#include <stdaliases.h>
#include <io/nativeio.h>
#include <cmds/path.h>
#include <containers/array.h>

#include "PBRTSceneDefs.h"

namespace cb
{

template <class TAllocator>
class PlyLoader
{
	public:
		PlyLoader(TAllocator* i_allocator);
		~PlyLoader();

		void									LoadFromFile(const floral::path& i_path);

	private:
		floral::file_info						m_PlyFile;
		floral::file_stream						m_DataStream;

		baker::Vec3Array*						m_Positions;
		baker::Vec3Array*						m_Normals;
		baker::Vec2Array*						m_UVs;
		baker::S32Array*						m_Indices;

		TAllocator*								m_Allocator;
};

}

#include "PlyLoader.inl"
