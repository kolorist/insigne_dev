#pragma once

#include <floral/stdaliases.h>

#include "Graphics/Tests/ITestSuite.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace perf
{
// ------------------------------------------------------------------

class Empty : public ITestSuite
{
public:
	static constexpr const_cstr k_name			= "empty";

public:
	Empty();
	~Empty();

	const_cstr									GetName() const override;

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return nullptr; }
};

// ------------------------------------------------------------------
}
}
