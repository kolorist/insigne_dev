#pragma once

#include <floral/stdaliases.h>
#include <floral/math/rng.h>

#include <insigne/commons.h>

#include "Graphics/Tests/ITestSuite.h"
#include "Graphics/FreeCamera.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace tools
{

class SurfelsGenerator : public ITestSuite
{
public:
	SurfelsGenerator();
	~SurfelsGenerator();

	const_cstr									GetName() const override;

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	struct SceneData
	{
		floral::mat4x4f							WVP;
	};

private:
	SceneData									m_SceneData;

private:
	floral::rng									m_RNG;

private:
	insigne::ub_handle_t						m_UB;
	FreeCamera									m_CameraMotion;
	MemoryArena*								m_MemoryArena;

	// resource control
private:
	ssize										m_BuffersBeginStateId;
	ssize										m_ShadingBeginStateId;
	ssize										m_TextureBeginStateId;
	ssize										m_RenderBeginStateId;
};

}
}
