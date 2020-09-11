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

struct Atmosphere
{
	f32											TopRadius;
	f32											BottomRadius;
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
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;
};

// ------------------------------------------------------------------
}
}
