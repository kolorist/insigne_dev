#include <clover.h>

namespace cb
{

template <class TAllocator>
PlyLoader<TAllocator>::PlyLoader(TAllocator* i_allocator)
	: m_Allocator(i_allocator)
{
	m_Positions = m_Allocator->allocate<baker::Vec3Array>(256u, m_Allocator);
	m_Normals = m_Allocator->allocate<baker::Vec3Array>(256u, m_Allocator);
	m_UVs = m_Allocator->allocate<baker::Vec2Array>(256u, m_Allocator);
	m_Indices = m_Allocator->allocate<baker::S32Array>(256u, m_Allocator);
}

template <class TAllocator>
PlyLoader<TAllocator>::~PlyLoader()
{
}

template <class TAllocator>
void PlyLoader<TAllocator>::LoadFromFile(const floral::path& i_path)
{
	m_PlyFile = floral::open_file(i_path);
	m_DataStream.buffer = (p8)m_Allocator->allocate(m_PlyFile.file_size);
	floral::read_all_file(m_PlyFile, m_DataStream);
	floral::close_file(m_PlyFile);

	c8 buffer[128];
	u32 vertexCount = 0;
	u32 faceCount = 0;
	bool hasNormal = false;
	m_DataStream.read_line_to_buffer(buffer);	// ply
	m_DataStream.read_line_to_buffer(buffer);	// endianess
	{
		m_DataStream.read_line_to_buffer(buffer);	// element vertex
		sscanf(buffer, "element vertex %d", &vertexCount);
	}

	m_DataStream.read_line_to_buffer(buffer);	// pos x
	m_DataStream.read_line_to_buffer(buffer);	// pos y
	m_DataStream.read_line_to_buffer(buffer);	// pos z

	{
		m_DataStream.read_line_to_buffer(buffer);	// normal or texcoord?
		c8 tmp[3];
		sscanf(buffer, "property float %s", tmp);
		if (strcmp(tmp, "nx") == 0)
		{
			hasNormal = true;
			m_DataStream.read_line_to_buffer(buffer);	// normal y
			m_DataStream.read_line_to_buffer(buffer);	// normal z
			m_DataStream.read_line_to_buffer(buffer);	// 
		}
		// the last one is alway texcoord v
		m_DataStream.read_line_to_buffer(buffer);	// texcoord v
	}

	{
		m_DataStream.read_line_to_buffer(buffer);	// element face
		sscanf(buffer, "element face %d", &faceCount);
	}
	m_DataStream.read_line_to_buffer(buffer);	// property list
	m_DataStream.read_line_to_buffer(buffer);	// end_header

	// vertex data
	for (u32 i = 0; i < vertexCount; i++)
	{
		floral::vec3f pos(0.0f);
		floral::vec3f norm(0.0f);
		floral::vec2f uv(0.0f);
		m_DataStream.read(&pos.x);
		m_DataStream.read(&pos.y);
		m_DataStream.read(&pos.z);
		if (hasNormal)
		{
			m_DataStream.read(&norm.x);
			m_DataStream.read(&norm.y);
			m_DataStream.read(&norm.z);
		}
		m_DataStream.read(&uv.x);
		m_DataStream.read(&uv.y);
		m_Positions->push_back(pos);
		m_Normals->push_back(norm);
		m_UVs->push_back(uv);
	}

	// indices
	for (u32 i = 0; i < faceCount; i++)
	{
		u8 indicesPerFace = 0;
		m_DataStream.read(&indicesPerFace);
		FLORAL_ASSERT(indicesPerFace == 3);
		for (u8 j = 0; j < indicesPerFace; j++)
		{
			s32 index = 0;
			m_DataStream.read(&index);
			m_Indices->push_back(index);
		}
	}
	FLORAL_ASSERT(m_DataStream.is_eos());
}

}
