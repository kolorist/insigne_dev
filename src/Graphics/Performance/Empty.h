#pragma once

#include <floral/stdaliases.h>

#include "Graphics/Tests/ITestSuite.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace perf
{

class Empty : public ITestSuite
{
public:
	Empty();
	~Empty();

	const_cstr									GetName() const;

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return nullptr; }
};

}
}
