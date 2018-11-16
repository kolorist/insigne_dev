#pragma once

#include <floral.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/DebugDrawer.h"

namespace stone {

class FormFactorsBaking : public ITestSuite {
	public:
		FormFactorsBaking();
		~FormFactorsBaking();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

		ICameraMotion*							GetCameraMotion() override { return nullptr; }

	private:
		DebugDrawer								m_DebugDrawer;

		floral::camera_view_t					m_CamView;
		floral::camera_persp_t					m_CamProj;
		floral::mat4x4f							m_WVP;

		LinearArena*							m_MemoryArena;
};

}
