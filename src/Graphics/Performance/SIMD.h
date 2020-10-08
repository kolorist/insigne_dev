#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace perf
{
// ------------------------------------------------------------------

class SIMD : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "simd";

public:
	SIMD();
	~SIMD();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	f32*										m_FloatArray;

private:
	LinearArena*								m_MemoryArena;
};

// ------------------------------------------------------------------
}
}
