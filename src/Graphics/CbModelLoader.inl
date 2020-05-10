#include <floral/assert/assert.h>
#include <floral/io/nativeio.h>
#include <floral/math/utils.h>

namespace cbmodel
{
// ------------------------------------------------------------------

template <class TVertex, class TIOAllocator, class TDataAllocator>
const Model<TVertex> LoadModelData(const floral::path& i_path, const VertexAttribute i_vtxAttrib, TIOAllocator* i_ioAllocator, TDataAllocator* i_dataAllocator)
{
	floral::file_info inp = floral::open_file(i_path);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)i_ioAllocator->allocate(inp.file_size);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);

	ModelHeader header;
	inpStream.read(&header);

	s32 materialNameLen = 0;
	inpStream.read(&materialNameLen);
	cstr materialName = (cstr)i_dataAllocator->allocate(materialNameLen + 1);
	memset(materialName, 0, materialNameLen + 1);
	inpStream.read_bytes((voidptr)materialName, materialNameLen);

	Model<TVertex> modelData;
	modelData.indicesCount = header.indicesCount;
	modelData.verticesCount = header.verticesCount;
	modelData.indicesData = nullptr;
	modelData.verticesData = nullptr;
	modelData.materialName = materialName;

	size vtxStride = 0;
	if (floral::test_bit_mask(i_vtxAttrib, VertexAttribute::Position))
	{
		vtxStride += sizeof(floral::vec3f);
	}
	if (floral::test_bit_mask(i_vtxAttrib, VertexAttribute::Normal))
	{
		vtxStride += sizeof(floral::vec3f);
	}
	if (floral::test_bit_mask(i_vtxAttrib, VertexAttribute::Tangent))
	{
		vtxStride += sizeof(floral::vec3f);
	}
	if (floral::test_bit_mask(i_vtxAttrib, VertexAttribute::TexCoord))
	{
		vtxStride += sizeof(floral::vec2f);
	}

	FLORAL_ASSERT(vtxStride == sizeof(TVertex));

	voidptr indicesData = i_dataAllocator->allocate(sizeof(s32) * header.indicesCount);
	voidptr verticesData = i_dataAllocator->allocate(vtxStride * header.verticesCount);

	// index
	inpStream.seek_begin(header.indicesOffset);
	inpStream.read_bytes(indicesData, header.indicesCount * sizeof(s32));

	// vertex
	floral::vec3f minCorner(9999.0f, 9999.0f, 9999.0f);
	floral::vec3f maxCorner(-9999.0f, -9999.0f, -9999.0f);
	aptr startOffset = 0;
	if (floral::test_bit_mask(i_vtxAttrib, VertexAttribute::Position))
	{
		aptr offset = startOffset;
		inpStream.seek_begin(header.positionOffset);
		for (s32 i = 0; i < header.verticesCount; i++)
		{
			voidptr dest = voidptr((aptr)verticesData + offset);
			inpStream.read_bytes(dest, sizeof(floral::vec3f));
			floral::vec3f* v = (floral::vec3f*)(dest);
			if (v->x > maxCorner.x) maxCorner.x = v->x;
			if (v->x < minCorner.x) minCorner.x = v->x;
			if (v->y > maxCorner.y) maxCorner.y = v->y;
			if (v->y < minCorner.y) minCorner.y = v->y;
			if (v->z > maxCorner.z) maxCorner.z = v->z;
			if (v->z < minCorner.z) minCorner.z = v->z;
			offset += vtxStride;
		}
		startOffset += sizeof(floral::vec3f);
	}
	modelData.aabb.max_corner = maxCorner;
	modelData.aabb.min_corner = minCorner;

	if (floral::test_bit_mask(i_vtxAttrib, VertexAttribute::Normal))
	{
		aptr offset = startOffset;
		inpStream.seek_begin(header.normalOffset);
		for (s32 i = 0; i < header.verticesCount; i++)
		{
			inpStream.read_bytes(voidptr((aptr)verticesData + offset), sizeof(floral::vec3f));
			offset += vtxStride;
		}
		startOffset += sizeof(floral::vec3f);
	}

	if (floral::test_bit_mask(i_vtxAttrib, VertexAttribute::Tangent))
	{
		aptr offset = startOffset;
		inpStream.seek_begin(header.tangentOffset);
		for (s32 i = 0; i < header.verticesCount; i++)
		{
			inpStream.read_bytes(voidptr((aptr)verticesData + offset), sizeof(floral::vec3f));
			offset += vtxStride;
		}
		startOffset += sizeof(floral::vec3f);
	}

	if (floral::test_bit_mask(i_vtxAttrib, VertexAttribute::TexCoord))
	{
		aptr offset = startOffset;
		inpStream.seek_begin(header.texcoordOffset);
		for (s32 i = 0; i < header.verticesCount; i++)
		{
			inpStream.read_bytes(voidptr((aptr)verticesData + offset), sizeof(floral::vec2f));
			offset += vtxStride;
		}
		startOffset += sizeof(floral::vec2f);
	}

	modelData.indicesData = (s32*)indicesData;
	modelData.verticesData = (TVertex*)verticesData;
	return modelData;
}

// ------------------------------------------------------------------
}
