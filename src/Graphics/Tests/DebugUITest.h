#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"
#include "Graphics/IDebugUI.h"

#include "Memory/MemorySystem.h"

namespace stone {

class DebugUITest : public ITestSuite, public IDebugUI
{
public:
	DebugUITest();
	~DebugUITest();

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnDebugUIUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return nullptr; }

private:
	LinearArena*								m_MemoryArena;
};

}
