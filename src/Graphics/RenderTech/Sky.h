#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace tech
{
// ------------------------------------------------------------------

struct DensityProfileLayer
{
	f32											Width;

	f32											ExpTerm;
	f32											ExpScale;
	f32											LinearTerm;
	f32											ConstantTerm;
};

struct DensityProfile
{
	DensityProfileLayer							Layers[2];
};

struct Atmosphere
{
	f32											TopRadius;
	f32											BottomRadius;

	floral::vec3f								RayleighScattering;
	DensityProfile								RayleighDensity;
	floral::vec3f								MieExtinction;
	DensityProfile								MieDensity;
	floral::vec3f								AbsorptionExtinction;
	DensityProfile								AbsorptionDensity;
};

// ------------------------------------------------------------------

class Sky : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "sky";

public:
	Sky();
	~Sky();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	Atmosphere									m_Atmosphere;

private:
	LinearArena*								m_DataArena;
	LinearArena*								m_TemporalArena;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;
};

// ------------------------------------------------------------------
}
}
