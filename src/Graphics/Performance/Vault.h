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
		floral::mat4x4f							viewProjectionMatrix;
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
	mat_loader::MaterialShaderPair				m_MSPair;

	SceneData									m_SceneData;
	insigne::ub_handle_t						m_SceneUB;

private:
	FreelistArena*								m_MemoryArena;
	LinearArena*								m_MaterialDataArena;
	LinearArena*								m_ModelDataArena;
};

// ------------------------------------------------------------------
}
}
