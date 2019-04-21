#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "Memory/MemorySystem.h"

namespace stone {

void											ComputeSH(f64* o_shr, f64* o_shg, f64* o_shb, f32* i_envMap);

//----------------------------------------------

class ProbeValidator
{
public:
	ProbeValidator();
	~ProbeValidator();

	void										Initialize();
	void										Setup(const floral::mat4x4f& i_XForm, floral::fixed_array<floral::vec3f, LinearAllocator>& i_shPositions);
	void										Validate(floral::simple_callback<void, const insigne::material_desc_t&> i_renderCb);
	const bool									IsValidationFinished() { return m_ValidationFinished; }
	const floral::fixed_array<floral::vec3f, LinearAllocator>&	GetValidatedLocations() { return m_ValidatedProbeLocs; }

private:
	struct SceneData {
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
	};

private:
	floral::fixed_array<floral::vec3f, LinearAllocator>	m_ProbeLocs;
	floral::fixed_array<floral::vec3f, LinearAllocator>	m_ValidatedProbeLocs;

	floral::fixed_array<SceneData, LinearAllocator>		m_ProbeSceneData;

	insigne::ub_handle_t						m_ProbeSceneDataUB;
	insigne::framebuffer_handle_t				m_ProbeRB;
	f32*										m_ProbePixelData;

	insigne::shader_handle_t					m_ValidationShader;
	insigne::material_desc_t					m_ValidationMaterial;

	bool										m_ValidationFinished;
	bool										m_CurrentProbeCaptured;
	u64											m_PixelDataReadyFrameIdx;
	u32											m_CurrentProbeIdx;
};

//----------------------------------------------

class SHBaker
{
public:
	SHBaker();
	~SHBaker();

	void										Initialize(const floral::mat4x4f& i_XForm, floral::fixed_array<floral::vec3f, LinearAllocator>& i_shPositions);
	void										SetupValidator(insigne::material_desc_t& io_material);
	void										Validate(insigne::material_desc_t& i_material,
													floral::simple_callback<void, const bool> i_renderCb);

private:
	struct SceneData {
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
	};

	struct SHProbeData {
		floral::mat4x4f							XForm;
		floral::vec4f							CoEffs[9];
	};

private:
	floral::fixed_array<floral::vec3f, LinearAllocator>		m_SHPositions;
	floral::fixed_array<SceneData, LinearAllocator>			m_EnvSceneData;
	floral::fixed_array<SHProbeData, LinearAllocator>		m_SHData;

	insigne::ub_handle_t						m_EnvMapSceneUB;
	insigne::framebuffer_handle_t				m_EnvMapRenderBuffer;
	f32*										m_EnvMapPixelData;

	bool										m_FinishValidation;
	bool										m_EnvCaptured;
	u64											m_FrameIdx;
	u32											m_EnvIdx;
};

}
