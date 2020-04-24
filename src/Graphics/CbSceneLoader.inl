namespace cbscene
{
// ------------------------------------------------------------------

template <class TIOAllocator, class TDataAllocator>
const Scene LoadSceneData(const floral::path& i_path, TIOAllocator* i_ioAllocator, TDataAllocator* i_dataAllocator)
{
	floral::file_info inp = floral::open_file(i_path);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)i_ioAllocator->allocate(inp.file_size);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);

	Scene newScene;

	SceneHeader header;
	inpStream.read(&header);

	newScene.nodesCount = header.nodesCount;
	newScene.nodeNames = i_dataAllocator->template allocate_array<const_cstr>(header.nodesCount);
	newScene.nodeFileNames = i_dataAllocator->template allocate_array<const_cstr>(header.nodesCount);
	newScene.nodeTransforms = i_dataAllocator->template allocate_array<NodeTransform>(header.nodesCount);

	for (size i = 0; i < header.nodesCount; i++)
	{
		s32 nodeNameLen = -1;
		inpStream.read(&nodeNameLen);
		cstr nodeName = (cstr)i_dataAllocator->allocate(nodeNameLen + 1);
		inpStream.read_bytes(nodeName, nodeNameLen);
		nodeName[nodeNameLen] = 0;

		s32 nodeFileNameLen = -1;
		inpStream.read(&nodeFileNameLen);
		cstr nodeFileName = (cstr)i_dataAllocator->allocate(nodeFileNameLen + 1);
		inpStream.read_bytes(nodeFileName, nodeFileNameLen);
		nodeFileName[nodeFileNameLen] = 0;

		NodeInfo nodeInfo;
		inpStream.read(&nodeInfo);

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
