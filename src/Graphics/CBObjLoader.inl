namespace cb
{

template <class TAllocator>
ModelLoader<TAllocator>::ModelLoader(TAllocator* i_allocator)
	: m_Allocator(i_allocator)
{
}

template <class TAllocator>
ModelLoader<TAllocator>::~ModelLoader()
{
}

template <class TAllocator>
void ModelLoader<TAllocator>::LoadFromFile(const floral::path& i_path)
{
	m_ModelFile = floral::open_file(i_path);
	m_DataStream.buffer = (p8)m_Allocator->allocate(m_ModelFile.file_size);
	floral::read_all_file(m_ModelFile, m_DataStream);
	floral::close_file(m_ModelFile);

	m_DataStream.read(&m_Header);

	for (u32 i = 0; i < m_Header.LODsCount; i++)
	{
		m_DataStream.read(&m_LODInfos[i]);
	}
}

template <class TAllocator>
const u32 ModelLoader<TAllocator>::GetVerticesCount(const u32 i_lodIndex)
{
	return m_LODInfos[i_lodIndex].VertexCount;
}

template <class TAllocator>
const u32 ModelLoader<TAllocator>::GetIndicesCount(const u32 i_lodIndex)
{
	return m_LODInfos[i_lodIndex].IndexCount;
}

template <class TAllocator>
template <class TArrayAllocator>
void ModelLoader<TAllocator>::ExtractPositionData(const u32 i_lodIndex, floral::fixed_array<floral::vec3f, TArrayAllocator>& o_posArray)
{
	ModelLODInfo& lodInfo = m_LODInfos[i_lodIndex];
	m_DataStream.seek_begin(m_Header.PositionOffset + sizeof(floral::vec3f) * lodInfo.OffsetToVertexData);
	for (u32 i = 0; i < lodInfo.VertexCount; i++)
	{
		floral::vec3f tmpVec(0.0f);
		m_DataStream.read(&tmpVec);
		o_posArray.push_back(tmpVec);
	}
}

template <class TAllocator>
void ModelLoader<TAllocator>::ExtractPositionData(const u32 i_lodIndex, const size i_stride, const size i_offset, voidptr o_target)
{
	ModelLODInfo& lodInfo = m_LODInfos[i_lodIndex];
	m_DataStream.seek_begin(m_Header.PositionOffset + sizeof(floral::vec3f) * lodInfo.OffsetToVertexData);
	aptr target = (aptr)o_target;
	for (u32 i = 0; i < lodInfo.VertexCount; i++)
	{
		m_DataStream.read_bytes((voidptr)(target + i_offset), sizeof(floral::vec3f));
		target += i_stride;
	}
}

template <class TAllocator>
void ModelLoader<TAllocator>::ExtractIndexData(const u32 i_lodIndex, const size i_stride, const size i_offset, voidptr o_target)
{
	ModelLODInfo& lodInfo = m_LODInfos[i_lodIndex];
	m_DataStream.seek_begin(m_Header.IndexOffset + sizeof(s32) * lodInfo.OffsetToIndexData);
	aptr target = (aptr)o_target;
	for (u32 i = 0; i < lodInfo.IndexCount; i++)
	{
		m_DataStream.read_bytes((voidptr)(target + i_offset), sizeof(s32));
		target += i_stride;
	}
}

}
