#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include <refrain2.h>

#include "Graphics/TestSuite.h"
#include "Memory/MemorySystem.h"

#include "precomputed_sky.h"

namespace stone
{
namespace tech
{
// ------------------------------------------------------------------

class Sky : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "sky";

private:
	struct SingleScatteringTaskData
	{
		Atmosphere*								atmosphere;
		f32**									deltaRayleighScatteringTexture;
		f32**									deltaMieScatteringTexture;
		f32**									scatteringTexture;
		f32*									transmittanceTexture;

		s32										currentDepth;
	};

	struct ScatteringDensityTaskData
	{
		Atmosphere*								atmosphere;
		f32**									deltaScatteringDensityTexture;

		f32**									deltaMultipleScatteringTexture;
		f32**									deltaRayleighScatteringTexture;
		f32**									deltaMieScatteringTexture;
		f32*									transmittanceTexture;
		f32*									deltaIrradianceTexture;

		s32										currentDepth;
		s32										scatteringOrder;
	};

	struct MultipleScatteringTaskData
	{
		Atmosphere*								atmosphere;
		f32*									transmittanceTexture;
		f32**									deltaScatteringDensityTexture;

		f32**									scatteringTexture;
		f32**									deltaMultipleScatteringTexture;

		s32										currentDepth;
	};

public:
	Sky();
	~Sky();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	static refrain2::Task						ComputeSingleScattering(voidptr i_data);
	static refrain2::Task						ComputeScatteringDensity(voidptr i_data);
	static refrain2::Task						ComputeMultipleScattering(voidptr i_data);

private:
	f32**										AllocateTexture3D(const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel);
	f32*										AllocateTexture2D(const s32 i_w, const s32 i_h, const s32 i_channel);
	void										WriteCacheTex2D(const_cstr i_cacheFileName, f32* i_data, const s32 i_w, const s32 i_h, const s32 i_channel);
	void										WriteCacheTex3D(const_cstr i_cacheFileName, f32** i_data, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel);
	void										WriteCache(const_cstr i_cacheFileName, voidptr i_data, const ssize i_size);
	f32*										LoadCacheTex2D(const_cstr i_cacheFileName, const s32 i_w, const s32 i_h, const s32 i_channel);
	f32**										LoadCacheTex3D(const_cstr i_cacheFileName, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel);
	voidptr										LoadCache(const_cstr i_cacheFileName, const ssize i_size, const voidptr i_buffer = nullptr);
	void										WriteRawTextureHDR2D(const_cstr i_texFileName, const ssize i_w, const ssize i_h, const s32 i_channel, f32* i_data);
	void										WriteRawTextureHDR3D(const_cstr i_texFileName, const ssize i_w, const ssize i_h, const s32 i_d, const s32 i_channel, f32** i_data);

	void										_DebugWriteHDR3D(const_cstr i_fileName, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel, f32** i_data);

private:
	Atmosphere									m_Atmosphere;

private:
	LinearArena*								m_DataArena;
	LinearArena*								m_TaskDataArena;
	helich::memory_region<LinearArena>			m_TexDataArenaRegion;
	LinearArena									m_TexDataArena;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;
};

// ------------------------------------------------------------------
}
}
