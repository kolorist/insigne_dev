#include "DebugRenderer.h"

#include "Camera.h"
#include "IMaterialManager.h"

namespace stone
{

DebugRenderer::DebugRenderer(IMaterialManager* i_materialManager)
	: m_MaterialManager(i_materialManager)
{
}

void DebugRenderer::Initialize()
{
	m_MemoryArena = g_PersistanceAllocator.allocate_arena<FreelistArena>(SIZE_MB(8));
	m_DebugLines = g_PersistanceAllocator.allocate<DebugVertices>(64u, m_MemoryArena);

	m_DebugRenderMat = m_MaterialManager->CreateMaterialFromFile(floral::path("gfx/mat/debug_render.mat"));
}

void DebugRenderer::DrawLine3D(const floral::vec3f& i_x0, const floral::vec3f& i_x1, const floral::vec4f& i_color)
{
	DebugDrawVertex v0, v1;
	v0.Position = i_x0;
	v0.Color = i_color;
	v1.Position = i_x1;
	v1.Color = i_color;

	m_DebugLines->PushBack(v0);
	m_DebugLines->PushBack(v1);
}

void DebugRenderer::Render(Camera* i_camera)
{
	// render lines
	m_DebugLines->Empty();
}

}
