#pragma once

#include <floral/stdaliases.h>

#include "Graphics/TestSuite.h"
#include "Graphics/InsigneHelpers.h"
#include "Graphics/PostFXChain.h"
#include "Graphics/MaterialLoader.h"

#include "Memory/MemorySystem.h"

namespace stone
{
namespace tech
{
// ------------------------------------------------------------------

class PBRHelmet : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "pbr helmet";

private:
	struct SceneData
	{
		floral::vec4f							cameraPos;
		floral::mat4x4f							viewProjectionMatrix;
		floral::vec4f							sh[9];
	};

public:
	PBRHelmet();
	~PBRHelmet();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;


private:
	helpers::SurfaceGPU							m_SurfaceGPU;
	mat_loader::MaterialShaderPair				m_MSPair;

	u64											m_frameIndex;
	f32											m_elapsedTime;
	floral::mat4x4f								m_projection, m_view;
	SceneData									m_SceneData;
	insigne::ub_handle_t						m_SceneUB;
	insigne::texture_handle_t					m_SplitSumTexture;

	pfx_chain::PostFXChain<LinearArena, FreelistArena>	m_PostFXChain;
private:
	FreelistArena*								m_MemoryArena;
	LinearArena*								m_MaterialDataArena;
	LinearArena*								m_ModelDataArena;
	LinearArena*								m_PostFXArena;
};

// ------------------------------------------------------------------
}
}
