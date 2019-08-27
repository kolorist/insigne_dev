#pragma once

#include <floral/stdaliases.h>
#include <floral/gpds/mat.h>

#include <insigne/commons.h>

#include "Graphics/Tests/ITestSuite.h"
#include "Graphics/FreeCamera.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace tools
{

class SingleAccurateFormFactor : public ITestSuite
{
public:
	SingleAccurateFormFactor();
	~SingleAccurateFormFactor();

	const_cstr									GetName() const;

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	void										_UpdateParallelVisual();
	void										_UpdatePerpendicularVisual();

private:
	struct SceneData
	{
		floral::mat4x4f							WVP;
	};

	struct ParallelConfigs
	{
		s32										SamplesCount;
		f32										Distance;
		bool									UseStratifiedSample;
	};

	struct PerpendicularConfigs
	{
		s32										SamplesCount;
		f32										Angle;
		bool									UseStratifiedSample;
	};

private:
	ParallelConfigs								m_ParaConfig;
	PerpendicularConfigs						m_PerpConfig;
	SceneData									m_SceneData;

//----------------------------------------------

private:
	insigne::ub_handle_t						m_UB;
	FreeCamera									m_CameraMotion;

	// resource control
private:
	ssize										m_BuffersBeginStateId;
	ssize										m_ShadingBeginStateId;
	ssize										m_TextureBeginStateId;
	ssize										m_RenderBeginStateId;
};

}
}
