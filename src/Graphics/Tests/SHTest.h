#pragma once

#include <refrain2.h>
#include <floral/stdaliases.h>

#include <atomic>

#include "ITestSuite.h"
#include "Graphics/IDebugUI.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/FreeCamera.h"

#include "Memory/MemorySystem.h"

namespace stone
{

class SHTest : public ITestSuite, public IDebugUI
{
public:
	SHTest();
	~SHTest();

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnDebugUIUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	struct SHComputeData
	{
		LinearArena* LocalMemoryArena;
		f32* InputTexture;
		f32* OutputRadianceTex;
		f32* OutputIrradianceTex;
		floral::vec3f OutputCoeffs[9];
		u32 Resolution;
	};

	SHComputeData								m_SHComputeTaskData;
	static refrain2::Task						ComputeSHCoeffs(voidptr i_data);

private:
	struct SceneData
	{
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
	};

private:
	SceneData									m_SceneData;

	insigne::ub_handle_t						m_UB;

	insigne::texture_handle_t					m_Texture;
	insigne::texture_handle_t					m_RadianceTexture;
	insigne::texture_handle_t					m_IrradianceTexture;
	f32*										m_TextureData;
	f32*										m_RadTextureData;
	f32*										m_IrrTextureData;
	u32											m_TextureResolution;
	bool										m_Computed;

	std::atomic<u32>							m_Counter;

private:
	DebugDrawer									m_DebugDrawer;
	FreeCamera									m_CameraMotion;
	LinearArena*								m_MemoryArena;
};

}
