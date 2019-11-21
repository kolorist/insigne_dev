#include <clover.h>
#include "CBObjDefinitions.h"

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
void PlyLoader<TAllocator>::LoadFromFile(const floral::path& i_path, const bool i_isAscii /* = false */)
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

	if (i_isAscii)
	{
		for (u32 i = 0; i < vertexCount; i++)
		{
			c8 lineBuffer[1024];
			m_DataStream.read_line_to_buffer(lineBuffer);
			floral::vec3f pos(0.0f);
			floral::vec3f norm(0.0f);
			floral::vec2f uv(0.0f);

			if (hasNormal)
			{
				s32 c = sscanf(lineBuffer, "%f %f %f %f %f %f %f %f",
						&pos.x, &pos.y, &pos.z,
						&norm.x, &norm.y, &norm.z,
						&uv.x, &uv.y);
				FLORAL_ASSERT(c == 8);
			}
			else
			{
				s32 c = sscanf(lineBuffer, "%f %f %f %f %f",
						&pos.x, &pos.y, &pos.z,
						&uv.x, &uv.y);
				FLORAL_ASSERT(c == 5);
			}
			m_Positions->push_back(pos);
			m_Normals->push_back(norm);
			m_UVs->push_back(uv);
		}

		for (u32 i = 0; i < faceCount; i++)
		{
			c8 lineBuffer[1024];
			m_DataStream.read_line_to_buffer(lineBuffer);

			s32 ipf = 0;
			s32 indices[3];
			s32 c = sscanf(lineBuffer, "%d %d %d %d",
					&ipf,
					&indices[0], &indices[1], &indices[2]);
			FLORAL_ASSERT(c == 4);

			m_Indices->push_back(indices[0]);
			m_Indices->push_back(indices[1]);
			m_Indices->push_back(indices[2]);
		}
	}
	else
	{
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
	}
	FLORAL_ASSERT(m_DataStream.is_eos());
}

template <class TAllocator>
void PlyLoader<TAllocator>::ConvertToCBOBJ(const_cstr i_filePath)
{
	FILE* outFile;
	outFile = fopen(i_filePath, "wb");
	cb::ModelDataHeader header;
	memset(&header, 0, sizeof(cb::ModelDataHeader));

	header.Version = 1u;
	header.LODsCount = 1;

	fwrite(&header, sizeof(cb::ModelDataHeader), 1, outFile);

	cb::ModelLODInfo lodInfo;
	lodInfo.LODIndex = 0;
	lodInfo.VertexCount = m_Positions->get_size();
	lodInfo.IndexCount = m_Indices->get_size();
	lodInfo.OffsetToIndexData = 0;
	lodInfo.OffsetToVertexData = 0;
	fwrite(&lodInfo, sizeof(cb::ModelLODInfo), 1, outFile);

	// write meshdata
	header.IndexOffset = ftell(outFile);
	fwrite(&(*m_Indices)[0], sizeof(s32), m_Indices->get_size(), outFile);
	header.PositionOffset = ftell(outFile);
	fwrite(&(*m_Positions)[0], sizeof(floral::vec3f), m_Positions->get_size(), outFile);
	header.NormalOffsets[0] = ftell(outFile);
	fwrite(&(*m_Normals)[0], sizeof(floral::vec3f), m_Normals->get_size(), outFile);
	header.TexCoordOffsets[0] = ftell(outFile);
	fwrite(&(*m_UVs)[0], sizeof(floral::vec2f), m_UVs->get_size(), outFile);

	fseek(outFile, 0, SEEK_SET);
	fwrite(&header, sizeof(cb::ModelDataHeader), 1, outFile);

	fclose(outFile);
}

}
