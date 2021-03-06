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
private:
	struct SceneData
	{
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
	};

	struct Patch
	{
		floral::vec3f							Vertex[4];
		floral::vec3f							Normal;
		floral::vec3f							Color;
		floral::vec3f							RadiosityColor;
		//floral::vec2<u32>						PixelCoord[4];
	};

	struct PixelVertex
	{
		floral::vec2<s32>						Coord;
		floral::vec3f							Color;
		floral::inplace_array<size, 4u>			PatchIdx;
	};

	struct FFRay
	{
		floral::vec3f							From, To;
		ssize									FromPatch, ToPatch;
		f32										Vis, GVis, GVisJ;
	};

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
	const bool									IsSegmentHitGeometry(const floral::vec3f& i_pi, const floral::vec3f& i_pj);
	void										CalculateFormFactors();
	void										CalculateRadiosity();
	void										UpdateLightmap();

	floral::vec3f								BilinearInterpolate(PixelVertex i_vtx[], floral::vec2<s32> i_pos);
	floral::vec3f								TestInterpolate(PixelVertex i_vtx[], floral::vec2<s32> i_pos, const s32 i_pid);

private:
	floral::fixed_array<Vertex3DPT, LinearAllocator>	m_RenderVertexData;
	floral::fixed_array<PixelVertex, LinearAllocator>	m_LightMapVertex;
	floral::fixed_array<s32, LinearAllocator>			m_LightMapIndex;
	floral::fixed_array<s32, LinearAllocator>		m_RenderIndexData;
	floral::fixed_array<Patch, LinearAllocator>		m_Patches;
	f32**										m_FF;
	floral::vec3f*								m_LightMapData;

	SceneData									m_SceneData;

	insigne::vb_handle_t					m_VB;
	insigne::ib_handle_t					m_IB;
	insigne::ub_handle_t					m_UB;
	insigne::shader_handle_t				m_Shader;
	insigne::material_desc_t				m_Material;
	insigne::texture_handle_t					m_LightMapTexture;

private:
	bool										m_DrawScene;
	bool										m_DrawFFPatches;
	bool										m_DrawFFRays;
	s32											m_SrcPatchIdx;
	s32											m_DstPatchIdx;
	s32											m_FFRayIdx;
	floral::fixed_array<FFRay, LinearAllocator>	m_DebugFFRays;

private:
	DebugDrawer								m_DebugDrawer;
	LinearArena*							m_MemoryArena;
	FreeCamera									m_CameraMotion;
};

}
