#pragma once

#include <floral/stdaliases.h>

#include "ITestSuite.h"
#include "Graphics/IDebugUI.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/FreeCamera.h"

namespace stone
{

class SHTest : public ITestSuite, public IDebugUI
{
public:
	SHTest();
	~SHTest();

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnDebugUIUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	struct SceneData {
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
	};

private:
	SceneData									m_SceneData;

	insigne::ub_handle_t						m_UB;
private:
	DebugDrawer									m_DebugDrawer;
	FreeCamera									m_CameraMotion;
};

}
