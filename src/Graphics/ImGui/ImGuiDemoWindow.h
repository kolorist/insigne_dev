#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace gui
{
// ------------------------------------------------------------------

class ImGuiDemoWindow : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "imgui demo window";

public:
	ImGuiDemoWindow();
	~ImGuiDemoWindow();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;
};

// ------------------------------------------------------------------
}
}
