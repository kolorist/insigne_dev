#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/FreeCamera.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/Tests/ITestSuite.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace tech
{

class FragmentPartition : public ITestSuite
{
public:
	FragmentPartition();
	~FragmentPartition();

	const_cstr									GetName() const override;

	void										OnInitialize(floral::filesystem<FreelistArena>* i_fs) override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	struct SceneData
	{
		floral::mat4x4f							WVP;
	};

	struct WorldData
	{
		floral::vec4f							BBMinCorner;
		floral::vec4f							BBDimension;
	};

	struct SHCoeffs
	{
		floral::vec4f							CoEffs[9];	// 3 bands
	};

	struct SHData
	{
		SHCoeffs								Probes[64];
	};

private:
	SceneData									m_SceneData;
	WorldData									m_WorldData;
	SHData*										m_SHData;
	FreeCamera									m_CameraMotion;
	floral::fixed_array<VertexP, LinearArena>	m_VerticesData;
	floral::fixed_array<s32, LinearArena>		m_IndicesData;

private:
	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;
	insigne::ub_handle_t						m_UB;
	insigne::ub_handle_t						m_SHUB;

	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;

private:
	LinearArena*								m_ResourceArena;
	FreelistArena*								m_TemporalArena;

	// resource control
private:
	ssize										m_BuffersBeginStateId;
	ssize										m_ShadingBeginStateId;
	ssize										m_TextureBeginStateId;
	ssize										m_RenderBeginStateId;
};

}
}
