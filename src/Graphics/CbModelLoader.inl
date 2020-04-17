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

	Model<TVertex> modelData;
	modelData.indicesCount = header.indicesCount;
	modelData.verticesCount = header.verticesCount;
	modelData.indicesData = nullptr;
	modelData.verticesData = nullptr;

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
	aptr startOffset = 0;
	if (floral::test_bit_mask(i_vtxAttrib, VertexAttribute::Position))
	{
		aptr offset = startOffset;
		inpStream.seek_begin(header.positionOffset);
		for (s32 i = 0; i < header.verticesCount; i++)
		{
			inpStream.read_bytes(voidptr((aptr)verticesData + offset), sizeof(floral::vec3f));
			offset += vtxStride;
		}
		startOffset += sizeof(floral::vec3f);
	}

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
