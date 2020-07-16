#pragma once

#include <floral/stdaliases.h>
#include <floral/cmds/path.h>
#include <floral/gpds/vec.h>
#include <floral/gpds/quaternion.h>

namespace cbscene
{
// ------------------------------------------------------------------

#pragma pack(push)
#pragma pack(1)
struct SceneHeader
{
	size										nodesCount;
};

struct NodeInfo
{
	floral::vec3f								translation;
	floral::quaternionf							rotation;
	floral::vec3f								scale;
};
#pragma pack(pop)

struct NodeTransform
{
	floral::vec3f								position;
	floral::quaternionf							rotation;
	floral::vec3f								scale;
};

struct Scene
{
	size										nodesCount;
	const_cstr*									nodeNames;
	const_cstr*									nodeFileNames;
	NodeTransform*								nodeTransforms;
};

template <class TIOAllocator, class TDataAllocator, class TFileSystem>
const Scene										LoadSceneData(TFileSystem* i_fs, const floral::relative_path& i_path, TIOAllocator* i_ioAllocator, TDataAllocator* i_dataAllocator);

template <class TIOAllocator, class TDataAllocator>
const Scene										LoadSceneData(const floral::path& i_path, TIOAllocator* i_ioAllocator, TDataAllocator* i_dataAllocator);

// ------------------------------------------------------------------
}

#include "CbSceneLoader.inl"
