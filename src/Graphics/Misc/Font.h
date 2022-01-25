#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Graphics/ImDrawer2D.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace misc
{
// ------------------------------------------------------------------

class Font : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "font";

public:
	Font();
	~Font();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
    ImDrawer2D                                  m_ImDrawer2D;

private:
	LinearArena*								m_MemoryArena;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;
};

// ------------------------------------------------------------------
}
}
