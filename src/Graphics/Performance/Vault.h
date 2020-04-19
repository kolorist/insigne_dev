#pragma once

#include <floral/stdaliases.h>

#include "Graphics/TestSuite.h"

#include "Graphics/InsigneHelpers.h"
#include "Graphics/MaterialLoader.h"

#include "Memory/MemorySystem.h"

namespace stone
{
namespace perf
{
// ------------------------------------------------------------------

class Vault : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "vault";

private:
	struct SceneData
	{
		floral::vec4f							cameraPos;
		floral::mat4x4f							viewProjectionMatrix;
		floral::vec4f							sh[9];
	};

public:
	Vault();
	~Vault();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;


private:
	helpers::SurfaceGPU							m_SurfaceGPU;
	helpers::SurfaceGPU							m_ScreenQuad;
	mat_loader::MaterialShaderPair				m_MSPair;
	mat_loader::MaterialShaderPair				m_SplitSumPair;

	SceneData									m_SceneData;
	insigne::ub_handle_t						m_SceneUB;
	insigne::texture_handle_t					m_CubeMapTex;

	insigne::framebuffer_handle_t				m_BrdfFB;

	bool										m_IsBakingSplitSum;

private:
	FreelistArena*								m_MemoryArena;
	LinearArena*								m_MaterialDataArena;
	LinearArena*								m_ModelDataArena;
};

// ------------------------------------------------------------------
}
}
