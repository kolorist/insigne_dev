#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace tools
{
// ------------------------------------------------------------------

class Noise : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "noise";

public:
	Noise();
	~Noise();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	FreelistArena*								m_MemoryArena;
};

// ------------------------------------------------------------------
}
}
