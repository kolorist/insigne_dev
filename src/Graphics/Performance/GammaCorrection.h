#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Graphics/InsigneHelpers.h"
#include "Graphics/MaterialLoader.h"

#include "Memory/MemorySystem.h"

namespace stone
{
namespace perf
{
// ------------------------------------------------------------------

class GammaCorrection : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "gamma correction";

public:
	GammaCorrection();
	~GammaCorrection();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	helpers::SurfaceGPU							m_Quad;
	mat_loader::MaterialShaderPair				m_MSPair;

private:
	FreelistArena*								m_MemoryArena;
	LinearArena*								m_MaterialDataArena;
};

// ------------------------------------------------------------------
}
}
