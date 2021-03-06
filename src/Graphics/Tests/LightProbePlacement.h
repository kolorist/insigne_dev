#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"
#include "SHBakingUtils.h"
#include "Memory/MemorySystem.h"
#include "Graphics/IDebugUI.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/FreeCamera.h"

namespace stone
{

class LightProbePlacement : public ITestSuite, public IDebugUI
{
public:
	LightProbePlacement();
	~LightProbePlacement();

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnDebugUIUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	void										RenderCallback(const insigne::material_desc_t& i_material);

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	void										DoScenePartition();
	const bool									CanStopPartition(floral::aabb3f& i_rootOctant);
	const bool									IsOctantTooSmall(floral::aabb3f& i_rootOctant);
	const bool									IsOctantHasTriangles(floral::aabb3f& i_rootOctant, const u32 i_threshold);
	void										Partition(floral::aabb3f& i_rootOctant);

	void										ExportSHData();

private:
	struct SHProbeData {
		floral::mat4x4f							XForm;
		floral::vec4f							CoEffs[9];
	};

private:
	floral::fixed_array<VertexPNC, LinearAllocator>		m_Vertices;
	floral::fixed_array<u32, LinearAllocator>			m_Indices;
	floral::fixed_array<GeoQuad, LinearAllocator>		m_Patches;

	floral::fixed_array<VertexP, LinearAllocator>	m_ProbeVertices;
	floral::fixed_array<u32, LinearAllocator>		m_ProbeIndices;
	floral::fixed_array<SHProbeData, LinearAllocator>		m_SHData;

	floral::aabb3f								m_SceneAABB;
	struct Octant
	{
		floral::aabb3f							Geometry;
		bool									IsLeaf;
		bool									HasTriangle;
	};
	floral::fixed_array<Octant, LinearAllocator>	m_Octants;
	floral::fixed_array<floral::vec3f, LinearAllocator>	m_ProbeLocations;

	struct SceneData {
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
	};

	bool										m_DrawCoordinate;
	bool										m_DrawProbeLocations;
	s32											m_DrawProbeRangeMin;
	s32											m_DrawProbeRangeMax;
	bool										m_DrawOctree;
	bool										m_DrawScene;
	bool										m_DrawSHProbes;
	bool										m_ProbePlacement;
	bool										m_SHBaking;

	SceneData									m_SceneData;

	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;
	insigne::ub_handle_t						m_UB;
	insigne::shader_handle_t					m_Shader;
	insigne::shader_handle_t					m_ValidationShader;
	insigne::material_desc_t					m_Material;
	insigne::material_desc_t					m_ValidationMaterial;

	insigne::vb_handle_t						m_ProbeVB;
	insigne::ib_handle_t						m_ProbeIB;
	insigne::ub_handle_t						m_ProbeUB;
	insigne::shader_handle_t					m_ProbeShader;
	insigne::material_desc_t					m_ProbeMaterial;

	LinearArena*								m_MemoryArena;
	ProbeValidator								m_ProbeValidator;
	DebugDrawer									m_DebugDrawer;
	FreeCamera									m_CameraMotion;
};

}
