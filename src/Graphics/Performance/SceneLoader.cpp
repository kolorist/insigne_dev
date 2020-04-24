#include "SceneLoader.h"

#include <clover/Logger.h>
#include <insigne/ut_render.h>

#include "InsigneImGui.h"

#include "Graphics/CbSceneLoader.h"

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------


SceneLoader::SceneLoader()
{
}

SceneLoader::~SceneLoader()
{
}

ICameraMotion* SceneLoader::GetCameraMotion()
{
	return nullptr;
}

const_cstr SceneLoader::GetName() const
{
	return k_name;
}

void SceneLoader::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(16));
	m_SceneDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(16));

	m_MemoryArena->free_all();
	const cbscene::Scene scene = cbscene::LoadSceneData(floral::path("gfx/go/models/demo/sponza/sponza.gltf.cbscene"),
			m_MemoryArena, m_SceneDataArena);

	m_ModelDataArray.reserve(scene.nodesCount, m_SceneDataArena);
	for (size i = 0; i < scene.nodesCount; i++)
	{
		c8 modelFile[512];
		sprintf(modelFile, "gfx/go/models/demo/sponza/%s", scene.nodeFileNames[i]);
		m_MemoryArena->free_all();
		cbmodel::Model<geo3d::VertexPNT> model = cbmodel::LoadModelData<geo3d::VertexPNT>(
				floral::path(modelFile),
				cbmodel::VertexAttribute::Position | cbmodel::VertexAttribute::Normal | cbmodel::VertexAttribute::TexCoord,
				m_MemoryArena, m_SceneDataArena);
		m_ModelDataArray.push_back(model);
	}
}

void SceneLoader::_OnUpdate(const f32 i_deltaMs)
{
}

void SceneLoader::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void SceneLoader::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	g_StreammingAllocator.free(m_SceneDataArena);
	g_StreammingAllocator.free(m_MemoryArena);
}

//-------------------------------------------------------------------
}
}
