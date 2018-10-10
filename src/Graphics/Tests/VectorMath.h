#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/DebugDrawer.h"

namespace stone {

class VectorMath : public ITestSuite {
	public:
		VectorMath();
		~VectorMath();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

	private:
		DebugDrawer								m_DebugDrawer;

		floral::fixed_array<floral::vec3f, LinearArena> m_Cube;
		floral::fixed_array<floral::vec3f, LinearArena> m_TranslationCube;
		floral::fixed_array<floral::vec3f, LinearArena> m_RotationCube;
		floral::fixed_array<floral::vec3f, LinearArena> m_ScaleCube;
		floral::fixed_array<floral::vec3f, LinearArena> m_XFormCube;

		LinearArena*							m_MemoryArena;

		floral::camera_view_t					m_CamView;
		floral::camera_persp_t					m_CamProj;
		floral::mat4x4f							m_WVP;
};

}
