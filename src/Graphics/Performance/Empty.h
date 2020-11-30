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

class Empty : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "empty";

public:
	Empty();
	~Empty();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	helich::memory_region<LinearArena>			m_DataArenaRegion;
	LinearArena									m_DataArena;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;
};

// ------------------------------------------------------------------
}
}
