#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Graphics/MaterialLoader.h"

#include "Memory/MemorySystem.h"

namespace stone
{
namespace perf
{
// ------------------------------------------------------------------

class Triangle : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "triangle";

public:
	Triangle();
	~Triangle();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;

	mat_loader::MaterialShaderPair				m_MSPair;

private:
	FreelistArena*								m_MemoryArena;
	LinearArena*								m_MaterialDataArena;
};

// ------------------------------------------------------------------
}
}
