#pragma once

#include <floral.h>

#include <insigne/commons.h>

#include "Memory/MemorySystem.h"
#include "IDebugRenderer.h"
#include "SurfaceDefinitions.h"

namespace stone
{

class IMaterialManager;

class DebugRenderer : public IDebugRenderer {
	private:
		typedef floral::dynamic_array<DebugDrawVertex, FreelistArena> DebugVertices;

	public:
		DebugRenderer(IMaterialManager* i_materialManager);
		
		void Initialize() override;
		void DrawLine3D(const floral::vec3f& i_x0, const floral::vec3f& i_x1, const floral::vec4f& i_color) override;
		void Render(Camera* i_camera) override;

	private:
		IMaterialManager*						m_MaterialManager;

		FreelistArena*							m_MemoryArena;

		DebugVertices*							m_DebugLines;
		insigne::material_handle_t				m_DebugRenderMat;
};

}
