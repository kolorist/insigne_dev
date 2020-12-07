#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Graphics/InsigneHelpers.h"
#include "Graphics/MaterialLoader.h"

#include "Memory/MemorySystem.h"

namespace stone
{
namespace tech
{
// ------------------------------------------------------------------

class SkyRuntime : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "sky runtime";

private:
	struct SceneData
	{
		floral::mat4x4f							modelFromView;
		floral::mat4x4f							viewFromClip;
	};

public:
	SkyRuntime();
	~SkyRuntime();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	insigne::texture_handle_t					LoadRawHDRTexture2D(const_cstr i_texFile, const s32 i_w, const s32 i_h, const s32 i_channel);
	insigne::texture_handle_t					LoadRawHDRTexture3D(const_cstr i_texFile, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel);

private:
	SceneData									m_SceneData;

	helpers::SurfaceGPU							m_Quad;

	mat_loader::MaterialShaderPair				m_MSPair;
	insigne::texture_handle_t					m_TransmittanceTexture;
	insigne::texture_handle_t					m_ScatteringTexture;
	insigne::texture_handle_t					m_IrradianceTexture;

	insigne::ub_handle_t						m_SceneUB;

private:
	helich::memory_region<LinearArena>			m_TexDataArenaRegion;
	LinearArena									m_TexDataArena;

	FreelistArena*								m_MemoryArena;
	LinearArena*								m_MaterialDataArena;
};

// ------------------------------------------------------------------
}
}
