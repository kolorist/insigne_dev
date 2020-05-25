#include "Memory/MemorySystem.h"

#include <floral/io/nativeio.h>

namespace stone
{

template <class TAllocator>
PlyData<TAllocator> LoadFromPly(const floral::path& i_path, TAllocator* i_allocator)
{
	floral::file_info plyFile = floral::open_file(i_path);
	floral::file_stream dataStream;

	g_TemporalLinearArena.free_all();

	dataStream.buffer = (p8)g_TemporalLinearArena.allocate(plyFile.file_size);
	floral::read_all_file(plyFile, dataStream);
	floral::close_file(plyFile);

	c8 buffer[1024];
	u32 vertexCount = 0;
	u32 faceCount = 0;
	//bool hasNormal = false;

	dataStream.read_line_to_buffer(buffer);	// ply
	dataStream.read_line_to_buffer(buffer);	// format ascii 1.0
	dataStream.read_line_to_buffer(buffer);	// comment
	dataStream.read_line_to_buffer(buffer);	// element vertex
	sscanf(buffer, "element vertex %d", &vertexCount);

	dataStream.read_line_to_buffer(buffer);	// pos x
	dataStream.read_line_to_buffer(buffer);	// pos y
	dataStream.read_line_to_buffer(buffer);	// pos z

	dataStream.read_line_to_buffer(buffer);	// nx
	dataStream.read_line_to_buffer(buffer);	// ny
	dataStream.read_line_to_buffer(buffer);	// nz

	dataStream.read_line_to_buffer(buffer);	// s
	dataStream.read_line_to_buffer(buffer);	// t

	dataStream.read_line_to_buffer(buffer);	// red
	dataStream.read_line_to_buffer(buffer);	// green
	dataStream.read_line_to_buffer(buffer);	// blue

	dataStream.read_line_to_buffer(buffer);	// element face
	sscanf(buffer, "element face %d", &faceCount);

	dataStream.read_line_to_buffer(buffer);	// property list
	dataStream.read_line_to_buffer(buffer);	// end_header

	PlyData<TAllocator> retData;

	retData.Position.reserve(vertexCount, i_allocator);
	retData.Normal.reserve(vertexCount, i_allocator);
	retData.TexCoord.reserve(vertexCount, i_allocator);
	retData.Color.reserve(vertexCount, i_allocator);
	retData.Indices.reserve(faceCount * 3, i_allocator);

	// vertex data
	for (u32 i = 0; i < vertexCount; i++)
	{
		floral::vec3f pos(0.0f);
		floral::vec3f norm(0.0f);
		floral::vec2f uv(0.0f);

		floral::vec3<u32> icolor(0);
		floral::vec3f color(0.0f);

		dataStream.read_line_to_buffer(buffer);	// ascii
		sscanf(buffer, "%f %f %f %f %f %f %f %f %d %d %d",
				&pos.x, &pos.y, &pos.z,
				&norm.x, &norm.y, &norm.z,
				&uv.x, &uv.y,
				&icolor.x, &icolor.y, &icolor.z);
		color.x = icolor.x / 255.0f;
		color.y = icolor.y / 255.0f;
		color.z = icolor.z / 255.0f;

		retData.Position.push_back(pos);
		retData.Normal.push_back(norm);
		retData.TexCoord.push_back(uv);
		retData.Color.push_back(color);
	}

	// indices
	for (u32 i = 0; i < faceCount; i++)
	{
		s32 indicesPerFace = 0;
		s32 faceIndices[3];
		dataStream.read_line_to_buffer(buffer);	// ascii
		sscanf(buffer, "%d %d %d %d", &indicesPerFace, &faceIndices[0], &faceIndices[1], &faceIndices[2]);
		FLORAL_ASSERT(indicesPerFace == 3);

		retData.Indices.push_back(faceIndices[0]);
		retData.Indices.push_back(faceIndices[1]);
		retData.Indices.push_back(faceIndices[2]);
	}
	FLORAL_ASSERT(dataStream.is_eos());

	return retData;
}

//----------------------------------------------

template <class TAllocator>
PlyData<TAllocator> LoadFFPatchesFromPly(const floral::path& i_path, TAllocator* i_allocator)
{
	floral::file_info plyFile = floral::open_file(i_path);
	floral::file_stream dataStream;

	g_TemporalLinearArena.free_all();

	dataStream.buffer = (p8)g_TemporalLinearArena.allocate(plyFile.file_size);
	floral::read_all_file(plyFile, dataStream);
	floral::close_file(plyFile);

	c8 buffer[1024];
	u32 vertexCount = 0;
	u32 faceCount = 0;
	//bool hasNormal = false;

	dataStream.read_line_to_buffer(buffer);	// ply
	dataStream.read_line_to_buffer(buffer);	// format ascii 1.0
	dataStream.read_line_to_buffer(buffer);	// comment
	dataStream.read_line_to_buffer(buffer);	// element vertex
	sscanf(buffer, "element vertex %d", &vertexCount);

	dataStream.read_line_to_buffer(buffer);	// pos x
	dataStream.read_line_to_buffer(buffer);	// pos y
	dataStream.read_line_to_buffer(buffer);	// pos z

	dataStream.read_line_to_buffer(buffer);	// nx
	dataStream.read_line_to_buffer(buffer);	// ny
	dataStream.read_line_to_buffer(buffer);	// nz

	dataStream.read_line_to_buffer(buffer);	// s
	dataStream.read_line_to_buffer(buffer);	// t

	dataStream.read_line_to_buffer(buffer);	// red
	dataStream.read_line_to_buffer(buffer);	// green
	dataStream.read_line_to_buffer(buffer);	// blue

	dataStream.read_line_to_buffer(buffer);	// element face
	sscanf(buffer, "element face %d", &faceCount);

	dataStream.read_line_to_buffer(buffer);	// property list
	dataStream.read_line_to_buffer(buffer);	// end_header

	PlyData<TAllocator> retData;

	retData.Position.reserve(vertexCount, i_allocator);
	retData.Normal.reserve(vertexCount, i_allocator);
	retData.TexCoord.reserve(vertexCount, i_allocator);
	retData.Color.reserve(vertexCount, i_allocator);
	retData.Indices.reserve(faceCount * 4, i_allocator);

	// vertex data
	for (u32 i = 0; i < vertexCount; i++)
	{
		floral::vec3f pos(0.0f);
		floral::vec3f norm(0.0f);
		floral::vec2f uv(0.0f);

		floral::vec3<u32> icolor(0);
		floral::vec3f color(0.0f);

		dataStream.read_line_to_buffer(buffer);	// ascii
		sscanf(buffer, "%f %f %f %f %f %f %f %f %d %d %d",
				&pos.x, &pos.y, &pos.z,
				&norm.x, &norm.y, &norm.z,
				&uv.x, &uv.y,
				&icolor.x, &icolor.y, &icolor.z);
		color.x = icolor.x / 255.0f;
		color.y = icolor.y / 255.0f;
		color.z = icolor.z / 255.0f;

		retData.Position.push_back(pos);
		retData.Normal.push_back(norm);
		retData.TexCoord.push_back(uv);
		retData.Color.push_back(color);
	}

	// indices
	for (u32 i = 0; i < faceCount; i++)
	{
		s32 indicesPerFace = 0;
		s32 faceIndices[4];
		dataStream.read_line_to_buffer(buffer);	// ascii
		sscanf(buffer, "%d %d %d %d %d", &indicesPerFace, &faceIndices[0], &faceIndices[1], &faceIndices[2], &faceIndices[3]);
		FLORAL_ASSERT(indicesPerFace == 4);

		retData.Indices.push_back(faceIndices[0]);
		retData.Indices.push_back(faceIndices[1]);
		retData.Indices.push_back(faceIndices[2]);
		retData.Indices.push_back(faceIndices[3]);
	}
	FLORAL_ASSERT(dataStream.is_eos());

	return retData;
}

}
