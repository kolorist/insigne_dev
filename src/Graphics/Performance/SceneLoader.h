#pragma once

#include <floral/stdaliases.h>
#include <floral/containers/fast_array.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Graphics/CbModelLoader.h"
#include "Graphics/SurfaceDefinitions.h"

#include "Memory/MemorySystem.h"

namespace stone
{
namespace perf
{
// ------------------------------------------------------------------

class SceneLoader : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "scene loader";

public:
	SceneLoader();
	~SceneLoader();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	using ModelDataArray = floral::fast_fixed_array<cbmodel::Model<geo3d::VertexPNT>, LinearArena>;
	ModelDataArray								m_ModelDataArray;

private:
	FreelistArena*								m_MemoryArena;
	LinearArena*								m_SceneDataArena;
};

// ------------------------------------------------------------------
}
}
