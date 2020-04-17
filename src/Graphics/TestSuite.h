#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Tests/ITestSuite.h"

namespace stone
{
// ------------------------------------------------------------------

class TestSuite : public ITestSuite
{
public:
	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

protected:
	virtual void								_OnInitialize() = 0;
	virtual void								_OnUpdate(const f32 i_deltaMs) = 0;
	virtual void								_OnRender(const f32 i_deltaMs) = 0;
	virtual void								_OnCleanUp() = 0;

	// resource control
private:
	ssize										m_BuffersBeginStateId;
	ssize										m_ShadingBeginStateId;
	ssize										m_TextureBeginStateId;
	ssize										m_RenderBeginStateId;
};

// ------------------------------------------------------------------
}
