#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "Memory/MemorySystem.h"

namespace stone {

void											ComputeSH(f64* o_shr, f64* o_shg, f64* o_shb, f32* i_envMap);

//----------------------------------------------
struct SHData {
	floral::vec4f								CoEffs[9];
};

class ProbeValidator
{
public:
	ProbeValidator();
	~ProbeValidator();

	void										Initialize();
	void										Setup(const floral::mat4x4f& i_XForm, floral::fixed_array<floral::vec3f, LinearAllocator>& i_shPositions);
	void										SetupGIMaterial(const insigne::material_desc_t& i_giMat) { m_GIMaterial = i_giMat; }
	void										Validate(floral::simple_callback<void, const insigne::material_desc_t&> i_renderCb);
	void										BakeSH(floral::simple_callback<void, const insigne::material_desc_t&> i_renderCb);
	const bool									IsValidationFinished() { return m_ValidationFinished; }
	const bool									IsSHBakingFinished() { return m_SHBakingFinished; }

	const floral::fixed_array<floral::vec3f, LinearAllocator>&	GetValidatedLocations() { return m_ValidatedProbeLocs; }
	const floral::fixed_array<SHData, LinearAllocator>&	GetValidatedSHData() { return m_ValidatedSHData; }

private:
	struct SceneData {
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
	};

private:
	floral::fixed_array<floral::vec3f, LinearAllocator>	m_ProbeLocs;
	floral::fixed_array<floral::vec3f, LinearAllocator>	m_ValidatedProbeLocs;
	floral::fixed_array<SHData, LinearAllocator>	m_ValidatedSHData;
	floral::fixed_array<u32, LinearAllocator>	m_ValidatedProbeDataIdx;

	floral::fixed_array<SceneData, LinearAllocator>		m_ProbeSceneData;

	insigne::ub_handle_t						m_ProbeSceneDataUB;
	insigne::framebuffer_handle_t				m_ProbeRB;
	f32*										m_ProbePixelData;

	insigne::shader_handle_t					m_ValidationShader;
	insigne::material_desc_t					m_ValidationMaterial;
	insigne::material_desc_t					m_GIMaterial;

	bool										m_ValidationFinished;
	bool										m_SHBakingFinished;
	bool										m_CurrentProbeCaptured;
	u64											m_PixelDataReadyFrameIdx;
	u32											m_CurrentProbeIdx;
	u32											m_CurrentSHIdx;
};

//----------------------------------------------

}
