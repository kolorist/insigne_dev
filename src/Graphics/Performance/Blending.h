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

class Blending : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "blending";

public:
	Blending(const_cstr i_pfxConfig);
	~Blending();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

protected:
	const_cstr									m_PostFXConfig;

private:
	bool										m_OptimizeOverdraw;

	helpers::SurfaceGPU							m_Quad0;
	helpers::SurfaceGPU							m_TQuad0;
	helpers::SurfaceGPU							m_TQuad00;
	helpers::SurfaceGPU							m_TQuad01;
	mat_loader::MaterialShaderPair				m_MSPairSolid;
	mat_loader::MaterialShaderPair				m_MSPairTransparent;

	pfx_chain::PostFXChain<LinearArena, FreelistArena>	m_PostFXChain;

private:
	FreelistArena*								m_MemoryArena;
	LinearArena*								m_MaterialDataArena;
	LinearArena*								m_PostFXArena;
};

// ------------------------------------------------------------------

class LDRBlending : public Blending
{
public:
	static constexpr const_cstr k_name			= "ldr blending";

public:
	LDRBlending()
		: Blending("ldr_postfx.pfx")
	{
	}

	~LDRBlending()
	{
	}

	const_cstr GetName() const override
	{
		return k_name;
	}
};

class HDRBlending : public Blending
{
public:
	static constexpr const_cstr k_name			= "hdr blending";

public:
	HDRBlending()
		: Blending("hdr_postfx.pfx")
	{
	}

	~HDRBlending()
	{
	}

	const_cstr GetName() const override
	{
		return k_name;
	}
};

// ------------------------------------------------------------------
}
}
