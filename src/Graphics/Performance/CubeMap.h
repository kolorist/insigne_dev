#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Graphics/InsigneHelpers.h"
#include "Graphics/MaterialLoader.h"
#include "Graphics/PostFXChain.h"

#include "Memory/MemorySystem.h"

namespace stone
{
namespace perf
{
// ------------------------------------------------------------------

class CubeMapTextures : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "cubemap textures";

public:
	CubeMapTextures();
	~CubeMapTextures();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	helpers::SurfaceGPU							m_Sphere;
	mat_loader::MaterialShaderPair				m_SphereMSPair;

	helpers::SurfaceGPU							m_SkySphere;
	mat_loader::MaterialShaderPair				m_SkyMSPair;

	pfx_chain::PostFXChain<LinearArena, FreelistArena>	m_PostFXChain;

private:
	struct SceneData
	{
		floral::mat4x4f							viewProjectionMatrix;
		floral::vec4f							cameraPosition;
		floral::vec4f							sh[9];
	};
	SceneData									m_SceneData;
	insigne::ub_handle_t						m_SceneUB;

private:
	FreelistArena*								m_MemoryArena;
	LinearArena*								m_MaterialDataArena;
	LinearArena*								m_PostFXArena;
};

// ------------------------------------------------------------------
}
}
