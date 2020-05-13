#pragma once

#include <floral/stdaliases.h>

#include "Graphics/TestSuite.h"
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
	cstr										m_WorkingDirCstr;

private:
	FreelistArena*								m_MemoryArena;
	FreelistArena*								m_FSMemoryArena;
};

// ------------------------------------------------------------------
}
}
