namespace cbscene
{
// ------------------------------------------------------------------

namespace details
{
// ------------------------------------------------------------------

template <class TIOAllocator, class TDataAllocator>
const Scene LoadSceneData(floral::file_stream& i_inpStream, TIOAllocator* i_ioAllocator, TDataAllocator* i_dataAllocator)
{
	Scene newScene;

	SceneHeader header;
	i_inpStream.read(&header);

	newScene.nodesCount = header.nodesCount;
	newScene.nodeNames = i_dataAllocator->template allocate_array<const_cstr>(header.nodesCount);
	newScene.nodeFileNames = i_dataAllocator->template allocate_array<const_cstr>(header.nodesCount);
	newScene.nodeTransforms = i_dataAllocator->template allocate_array<NodeTransform>(header.nodesCount);

	for (size i = 0; i < header.nodesCount; i++)
	{
		s32 nodeNameLen = -1;
		i_inpStream.read(&nodeNameLen);
		cstr nodeName = (cstr)i_dataAllocator->allocate(nodeNameLen + 1);
		i_inpStream.read_bytes(nodeName, nodeNameLen);
		nodeName[nodeNameLen] = 0;

		s32 nodeFileNameLen = -1;
		i_inpStream.read(&nodeFileNameLen);
		cstr nodeFileName = (cstr)i_dataAllocator->allocate(nodeFileNameLen + 1);
		i_inpStream.read_bytes(nodeFileName, nodeFileNameLen);
		nodeFileName[nodeFileNameLen] = 0;

		NodeInfo nodeInfo;
		i_inpStream.read(&nodeInfo);

		newScene.nodeNames[i] = nodeName;
		newScene.nodeFileNames[i] = nodeFileName;
		newScene.nodeTransforms[i].position = nodeInfo.translation;
		newScene.nodeTransforms[i].rotation = nodeInfo.rotation;
		newScene.nodeTransforms[i].scale = nodeInfo.scale;
	}

	return newScene;
}

// ------------------------------------------------------------------
}

template <class TIOAllocator, class TDataAllocator, class TFileSystem>
const Scene LoadSceneData(TFileSystem* i_fs, const floral::relative_path& i_path, TIOAllocator* i_ioAllocator, TDataAllocator* i_dataAllocator)
{
	floral::file_info inp = floral::open_file_read(i_fs, i_path);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)i_ioAllocator->allocate(inp.file_size);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);

	return details::LoadSceneData<TIOAllocator, TDataAllocator>(inpStream, i_ioAllocator, i_dataAllocator);
}

template <class TIOAllocator, class TDataAllocator>
const Scene LoadSceneData(const floral::path& i_path, TIOAllocator* i_ioAllocator, TDataAllocator* i_dataAllocator)
{
	floral::file_info inp = floral::open_file(i_path);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)i_ioAllocator->allocate(inp.file_size);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);

	return details::LoadSceneData<TIOAllocator, TDataAllocator>(inpStream, i_ioAllocator, i_dataAllocator);
}

// ------------------------------------------------------------------
}
