#pragma once

#include <floral.h>

#include "ITestSuite.h"
#include "Graphics/IDebugUI.h"
#include "Memory/MemorySystem.h"
#include "Graphics/FreeCamera.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

class FormFactorsValidating : public ITestSuite, public IDebugUI
{
public:
	FormFactorsValidating();
	~FormFactorsValidating();

	void									OnInitialize() override;
	void									OnUpdate(const f32 i_deltaMs) override;
	void									OnDebugUIUpdate(const f32 i_deltaMs) override;
	void									OnRender(const f32 i_deltaMs) override;
	void									OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	void									CalculateFormFactors_Regular();
	void									CalculateFormFactors_Stratified();	// aka 'Jittered' Sampling

private:
	struct SceneData {
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
	};

private:
	floral::fixed_array<floral::vec3f, LinearAllocator> m_Patch1Samples;
	floral::fixed_array<floral::vec3f, LinearAllocator> m_Patch2Samples;
	floral::fixed_array<size, LinearAllocator> m_Rays;

	SceneData									m_SceneData;

	insigne::vb_handle_t					m_VB;
	insigne::ib_handle_t					m_IB;
	insigne::ub_handle_t					m_UB;
	insigne::shader_handle_t				m_Shader;
	insigne::material_desc_t				m_Material;

private:
	DebugDrawer								m_DebugDrawer;
	LinearArena*							m_MemoryArena;
	FreeCamera									m_CameraMotion;
};

}
