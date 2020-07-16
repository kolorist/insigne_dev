#pragma once

#include <floral/stdaliases.h>
#include <floral/containers/fast_array.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Graphics/CbModelLoader.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/MaterialLoader.h"
#include "Graphics/InsigneHelpers.h"
#include "Graphics/PostFXChain.h"

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
	mat_loader::MaterialShaderPair				_LoadMaterial(const_cstr i_path);

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	struct ModelRegistry
	{
		cbmodel::Model<geo3d::VertexPNTT>		model;
		mat_loader::MaterialShaderPair			msPair;
		helpers::SurfaceGPU						modelGPU;
	};

	struct MaterialKeyPair
	{
		floral::crc_string						key;
		mat_loader::MaterialShaderPair			msPair;
	};

	struct SceneData
	{
		floral::mat4x4f							viewProjectionMatrix;
		floral::mat4x4f							shadowViewProjectionMatrix;
		floral::vec4f							cameraPosition;
		floral::vec4f							sh[9];
	};

	struct ShadowSceneData
	{
		floral::mat4x4f							viewProjectionMatrix;
	};

	struct LightingData
	{
		floral::vec4f							lightDirection;
		floral::vec4f							lightIntensity;
	};

private:
	using ModelDataArray = floral::fast_fixed_array<ModelRegistry, LinearArena>;
	using MaterialArray = floral::fast_fixed_array<MaterialKeyPair, LinearArena>;
	ModelDataArray								m_ModelDataArray;
	MaterialArray								m_MaterialArray;
	floral::aabb3f								m_SceneAABB;

	floral::mat4x4f								m_projection, m_view;
	SceneData									m_SceneData;
	LightingData								m_LightingData;
	insigne::ub_handle_t						m_SceneUB;
	insigne::ub_handle_t						m_LightingUB;
	insigne::texture_handle_t					m_SplitSumTexture;

	floral::vec3f								m_ShadowSceneAABB[8];
	floral::vec3f								m_ShadowCameraPosition;
	ShadowSceneData								m_ShadowSceneData;
	insigne::ub_handle_t						m_ShadowSceneUB;
	mat_loader::MaterialShaderPair				m_ShadowMapMaterial;
	insigne::framebuffer_handle_t				m_ShadowMapFb;

	pfx_chain::PostFXChain<LinearArena, FreelistArena>	m_PostFXChain;
	u64											m_frameIndex;

private:
	FreelistArena*								m_MemoryArena;
	LinearArena*								m_SceneDataArena;
	LinearArena*								m_MaterialDataArena;
	LinearArena*								m_PostFXArena;
};

// ------------------------------------------------------------------
}
}
