#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"
#include "SHBakingUtils.h"
#include "Memory/MemorySystem.h"
#include "Graphics/IDebugUI.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/DebugDrawer.h"

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

	ICameraMotion*								GetCameraMotion() override { return nullptr; }

private:
	void										DoScenePartition();
	const bool									CanStopPartition(floral::aabb3f& i_rootOctant);
	const bool									IsOctantTooSmall(floral::aabb3f& i_rootOctant);
	const bool									IsOctantHasTriangles(floral::aabb3f& i_rootOctant, const u32 i_threshold);
	void										Partition(floral::aabb3f& i_rootOctant);

private:
	floral::fixed_array<VertexPNC, LinearAllocator>		m_Vertices;
	floral::fixed_array<u32, LinearAllocator>			m_Indices;
	floral::fixed_array<GeoQuad, LinearAllocator>		m_Patches;

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

	SceneData									m_SceneData;

	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;
	insigne::ub_handle_t						m_UB;
	insigne::shader_handle_t					m_Shader;
	insigne::shader_handle_t					m_ValidationShader;
	insigne::material_desc_t					m_Material;
	insigne::material_desc_t					m_ValidationMaterial;

	floral::camera_view_t						m_CamView;
	floral::camera_persp_t						m_CamProj;

	LinearArena*								m_MemoryArena;
	ProbeValidator								m_ProbeValidator;
	DebugDrawer									m_DebugDrawer;
};

}
