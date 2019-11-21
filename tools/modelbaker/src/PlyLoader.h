#pragma once

#include <floral/stdaliases.h>
#include <floral/io/nativeio.h>
#include <floral/cmds/path.h>
#include <floral/containers/array.h>

#include "PBRTSceneDefs.h"

namespace cb
{

template <class TAllocator>
class PlyLoader
{
	public:
		PlyLoader(TAllocator* i_allocator);
		~PlyLoader();

		void									LoadFromFile(const floral::path& i_path, const bool i_isAscii = false);
		void									ConvertToCBOBJ(const_cstr i_filePath);

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
