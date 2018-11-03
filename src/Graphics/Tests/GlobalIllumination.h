#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/TrackballCamera.h"

namespace stone {

class GlobalIllumination : public ITestSuite {
	public:
		GlobalIllumination();
		~GlobalIllumination();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

		ICameraMotion*							GetCameraMotion() override { return &m_CameraMotion; }

	private:
		DebugDrawer								m_DebugDrawer;
		TrackballCamera							m_CameraMotion;

		floral::camera_view_t					m_CamView;
		floral::camera_persp_t					m_CamProj;

		LinearArena*							m_MemoryArena;
};

}
