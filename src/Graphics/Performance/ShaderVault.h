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

class ShaderVault : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "shader vault";

public:
	ShaderVault();
	~ShaderVault();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	struct SceneData
	{
		floral::vec4f							resolution;
		floral::vec4f							timeSeconds;
	};

private:
	SceneData									m_SceneData;
	insigne::ub_handle_t						m_SceneUB;

	helpers::SurfaceGPU							m_Quad;
	size										m_MaterialIndex;
	mat_loader::MaterialShaderPair				m_MSPair[5];

private:
	FreelistArena*								m_MemoryArena;
	LinearArena*								m_MaterialDataArena;
};

// ------------------------------------------------------------------
}
}
