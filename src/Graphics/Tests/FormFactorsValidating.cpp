#include "FormFactorsValidating.h"

#include <floral/comgeo/shapegen.h>

#include <insigne/commons.h>
#include <insigne/counters.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>
#include <clover.h>

#include <stdlib.h>
#include <time.h>
#include <random>

#include "Graphics/PlyLoader.h"
#include "Graphics/stb_image_write.h"

namespace stone {

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec2 l_TexCoord;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

out highp vec2 v_Texcoord;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_Texcoord = l_TexCoord;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D u_LightMapTex;

in highp vec2 v_Texcoord;

void main()
{
	o_Color = vec4(texture(u_LightMapTex, v_Texcoord).rgb, 1.0f);
}
)";

FormFactorsValidating::FormFactorsValidating()
	: m_CameraMotion(
			floral::camera_view_t { floral::vec3f(-0.3f, 1.0f, -3.0f), floral::vec3f(0.3f, -1.0f, 3.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
			floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_DrawScene(true)
	, m_DrawFFPatches(false)
	, m_SrcPatchIdx(0)
	, m_DstPatchIdx(0)
{
	srand(time(0));
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

FormFactorsValidating::~FormFactorsValidating()
{
}

void FormFactorsValidating::OnInitialize()
{
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_SceneData.WVP = m_CameraMotion.GetWVP();
		m_SceneData.XForm = floral::mat4x4f(1.0f);

		insigne::update_ub(newUB, &m_SceneData, sizeof(SceneData), 0);
		m_UB = newUB;
	}

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(256);
		desc.stride = sizeof(Vertex3DPT);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(128);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		m_IB = newIB;
	}


	{
		PlyData<LinearArena> plyData = LoadFromPly(floral::path("gfx/envi/models/demo/cornel_box_triangles_pp.ply"),
				m_MemoryArena);
		CLOVER_DEBUG("Render mesh loaded");

		size vtxCount = plyData.Position.get_size();
		size idxCount = plyData.Indices.get_size();

		CLOVER_DEBUG("Vertex count: %zd", vtxCount);
		CLOVER_DEBUG("Index count: %zd", idxCount);

		m_RenderVertexData.reserve(vtxCount, &g_StreammingAllocator);
		m_RenderIndexData.reserve(idxCount, &g_StreammingAllocator);

		for (size i = 0; i < vtxCount; i++)
		{
			Vertex3DPT vtx;
			vtx.Position = plyData.Position[i];
			vtx.TexCoord = plyData.TexCoord[i];
			m_RenderVertexData.push_back(vtx);
		}

		for (size i = 0; i < idxCount; i++)
		{
			m_RenderIndexData.push_back(plyData.Indices[i]);
		}

		insigne::copy_update_vb(m_VB, &m_RenderVertexData[0], vtxCount, sizeof(Vertex3DPT), 0);
		insigne::copy_update_ib(m_IB, &m_RenderIndexData[0], idxCount, 0);
	}

	{
		PlyData<LinearArena> plyData = LoadFFPatchesFromPly(floral::path("gfx/envi/models/demo/cornel_box_patches_pp.ply"),
				m_MemoryArena);
		CLOVER_DEBUG("GI mesh loaded");

		size vtxCount = plyData.Position.get_size();
		size idxCount = plyData.Indices.get_size();
		size patchCount = idxCount / 4;

		CLOVER_DEBUG("Vertex count: %zd", vtxCount);
		CLOVER_DEBUG("Index count: %zd - %zd patches", idxCount, idxCount / 4);

		m_Patches.reserve(idxCount / 4, &g_StreammingAllocator);

		const s32 k_giLightmapSize = 512;

		for (size i = 0; i < idxCount; i += 4)
		{
			s32 vtxIdx[4];
			vtxIdx[0] = plyData.Indices[i];
			vtxIdx[1] = plyData.Indices[i + 1];
			vtxIdx[2] = plyData.Indices[i + 2];
			vtxIdx[3] = plyData.Indices[i + 3];

			Patch patch;
			patch.Vertex[0] = plyData.Position[vtxIdx[0]];
			patch.Vertex[1] = plyData.Position[vtxIdx[1]];
			patch.Vertex[2] = plyData.Position[vtxIdx[2]];
			patch.Vertex[3] = plyData.Position[vtxIdx[3]];

			patch.Normal = plyData.Normal[vtxIdx[0]];

			patch.Color = plyData.Color[vtxIdx[0]];
			patch.RadiosityColor = floral::vec3f(0.0f);

			patch.PixelCoord[0] =
				floral::vec2<u32>((u32)round(plyData.TexCoord[vtxIdx[0]].x * k_giLightmapSize),
						(u32)round(plyData.TexCoord[vtxIdx[0]].y * k_giLightmapSize));
			patch.PixelCoord[1] =
				floral::vec2<u32>((u32)round(plyData.TexCoord[vtxIdx[1]].x * k_giLightmapSize),
						(u32)round(plyData.TexCoord[vtxIdx[1]].y * k_giLightmapSize));
			patch.PixelCoord[2] =
				floral::vec2<u32>((u32)round(plyData.TexCoord[vtxIdx[2]].x * k_giLightmapSize),
						(u32)round(plyData.TexCoord[vtxIdx[2]].y * k_giLightmapSize));
			patch.PixelCoord[3] =
				floral::vec2<u32>((u32)round(plyData.TexCoord[vtxIdx[3]].x * k_giLightmapSize),
						(u32)round(plyData.TexCoord[vtxIdx[3]].y * k_giLightmapSize));

			m_Patches.push_back(patch);
		}

		m_FF = g_StreammingAllocator.allocate_array<f32*>(patchCount);
		for (size i = 0; i < patchCount; i++)
		{
			m_FF[i] = g_StreammingAllocator.allocate_array<f32>(patchCount);
			memset(m_FF[i], 0, patchCount * sizeof(f32));
		}

		CalculateFormFactors();
		m_LightMapData = g_StreammingAllocator.allocate_array<floral::vec3f>(k_giLightmapSize * k_giLightmapSize);
		CalculateRadiosity();
		stbi_write_hdr("out2.hdr", k_giLightmapSize, k_giLightmapSize, 3, (f32*)m_LightMapData);
	}

	// upload radiosity texture
	{
		insigne::texture_desc_t texDesc;
		texDesc.width = 512;
		texDesc.height = 512;
		texDesc.format = insigne::texture_format_e::hdr_rgb;
		texDesc.min_filter = insigne::filtering_e::nearest;
		texDesc.mag_filter = insigne::filtering_e::nearest;
		texDesc.dimension = insigne::texture_dimension_e::tex_2d;
		texDesc.has_mipmap = false;
		const size dataSize = insigne::prepare_texture_desc(texDesc);
		p8 pData = (p8)texDesc.data;
		memcpy(pData, (p8)m_LightMapData, dataSize);
		m_LightMapTexture = insigne::create_texture(texDesc);
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_LightMapTex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/scene/sample_vs");
		desc.fs_path = floral::path("/scene/sample_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		// static uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Scene");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		}

		{
			s32 texSlot = insigne::get_material_texture_slot(m_Material, "u_LightMapTex");
			m_Material.textures[texSlot].value = m_LightMapTexture;
		}
	}

	m_DebugDrawer.Initialize();
}

void FormFactorsValidating::OnDebugUIUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("FormFactor Controller");
	if (ImGui::CollapsingHeader("FF Debug"))
	{
		ImGui::Checkbox("Draw Scene", &m_DrawScene);
		ImGui::Checkbox("Draw FF Patches", &m_DrawFFPatches);
		if (m_DrawFFPatches)
		{
			ImGui::InputInt("Source patch", &m_SrcPatchIdx);
			ImGui::InputInt("Destination patch", &m_DstPatchIdx);
			if (m_SrcPatchIdx >= 0 && m_DstPatchIdx >= 0 && m_SrcPatchIdx < m_Patches.get_size() && m_DstPatchIdx < m_Patches.get_size())
			{
				ImGui::Text("FF[%d, %d] = %f", m_SrcPatchIdx, m_DstPatchIdx, m_FF[m_SrcPatchIdx][m_DstPatchIdx]);
			}
			else
			{
				ImGui::Text("Invalid !!!");
			}
		}
	}
	ImGui::End();
}

void FormFactorsValidating::OnUpdate(const f32 i_deltaMs)
{
	m_CameraMotion.OnUpdate(i_deltaMs);

	m_DebugDrawer.BeginFrame();

	// ground grid cover [-2.0..2.0]
	floral::vec4f gridColor(0.3f, 0.4f, 0.5f, 1.0f);
	for (s32 i = -2; i < 3; i++)
	{
		for (s32 j = -2; j < 3; j++)
		{
			floral::vec3f startX(1.0f * i, 0.0f, 1.0f * j);
			floral::vec3f startZ(1.0f * j, 0.0f, 1.0f * i);
			floral::vec3f endX(1.0f * i, 0.0f, -1.0f * j);
			floral::vec3f endZ(-1.0f * j, 0.0f, 1.0f * i);
			m_DebugDrawer.DrawLine3D(startX, endX, gridColor);
			m_DebugDrawer.DrawLine3D(startZ, endZ, gridColor);
		}
	}

	// coordinate unit vectors
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.1f, 0.0f, 0.1f), floral::vec3f(0.5f, 0.0f, 0.1f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.1f, 0.0f, 0.1f), floral::vec3f(0.1f, 0.5f, 0.1f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.1f, 0.0f, 0.1f), floral::vec3f(0.1f, 0.0f, 0.5f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));

	// -----------------------------------------
	// debug draw
	if (m_DrawFFPatches)
	{
#if 0
		floral::vec3f vertices[] = {
			floral::vec3f(-1.0f, 1.0f, -0.6f),
			floral::vec3f(-1.0f, 1.0f, -0.4f),
			floral::vec3f(-1.0f, 0.8f, -0.4f),
			floral::vec3f(-1.0f, 0.8f, -0.6f)
		};
		floral::vec3f pi(-0.291400462f, 0.000196003763f, -0.236530572f);
		floral::vec3f n(0.410295993f, 0.0f, -0.911952019f);
		floral::vec3f pin = pi + n;

		m_DebugDrawer.DrawQuad3D(vertices[0], vertices[1], vertices[2], vertices[3], floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
		m_DebugDrawer.DrawLine3D(pi, vertices[0], floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
		m_DebugDrawer.DrawLine3D(pi, vertices[1], floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
		m_DebugDrawer.DrawLine3D(pi, vertices[2], floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
		m_DebugDrawer.DrawLine3D(pi, vertices[3], floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
		m_DebugDrawer.DrawLine3D(pi, pin, floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
#else
		if (m_SrcPatchIdx >= 0 && m_DstPatchIdx >= 0 && m_SrcPatchIdx < m_Patches.get_size() && m_DstPatchIdx < m_Patches.get_size())
		{
			const Patch& srcPatch = m_Patches[m_SrcPatchIdx];
			const Patch& dstPatch = m_Patches[m_DstPatchIdx];
			m_DebugDrawer.DrawQuad3D(srcPatch.Vertex[0], srcPatch.Vertex[1], srcPatch.Vertex[2], srcPatch.Vertex[3],
					floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
			m_DebugDrawer.DrawQuad3D(dstPatch.Vertex[0], dstPatch.Vertex[1], dstPatch.Vertex[2], dstPatch.Vertex[3],
					floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
		}
#endif
	}
	// -----------------------------------------

	m_DebugDrawer.EndFrame();
}

void FormFactorsValidating::OnRender(const f32 i_deltaMs)
{
	// camera
	m_SceneData.WVP = m_CameraMotion.GetWVP();
	insigne::update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	if (m_DrawScene)
	{
		insigne::draw_surface<Surface3DPT>(m_VB, m_IB, m_Material);
	}
	m_DebugDrawer.Render(m_SceneData.WVP);
	IDebugUI::OnFrameRender(i_deltaMs);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void FormFactorsValidating::OnCleanUp()
{
}

//----------------------------------------------

#define PId2     1.570796326794896619f   /* pi / 2 */
#define PIt2inv  0.159154943091895346f   /* 1 / (2 * pi) */

const f32 compute_accurate_point2patch_form_factor(
	const floral::vec3f i_quad[], const floral::vec3f& i_point, const floral::vec3f& i_pointNormal)
{
	floral::vec3f PA = i_quad[0] - i_point;
	floral::vec3f PB = i_quad[1] - i_point;
	floral::vec3f PC = i_quad[2] - i_point;
	floral::vec3f PD = i_quad[3] - i_point;

	floral::vec3f gAB = floral::cross(PA, PB);
	floral::vec3f gBC = floral::cross(PB, PC);
	floral::vec3f gCD = floral::cross(PC, PD);
	floral::vec3f gDA = floral::cross(PD, PA);

	f32 NdotGAB = floral::dot(i_pointNormal, gAB);
	f32 NdotGBC = floral::dot(i_pointNormal, gBC);
	f32 NdotGCD = floral::dot(i_pointNormal, gCD);
	f32 NdotGDA = floral::dot(i_pointNormal, gDA);

	f32 sum = 0.0f;

	if (fabs(NdotGAB) > 0.0f)
	{
		f32 lenGAB = floral::length(gAB);
		if (lenGAB > 0.0f)
		{
			f32 gammaAB = PId2 - atanf(floral::dot(PA, PB) / lenGAB);
			sum += NdotGAB * gammaAB / lenGAB;
		}
	}

	if (fabs(NdotGBC) > 0.0f)
	{
		f32 lenGBC = floral::length(gBC);
		if (lenGBC > 0.0f)
		{
			f32 gammaBC = PId2 - atanf(floral::dot(PB, PC) / lenGBC);
			sum += NdotGBC * gammaBC / lenGBC;
		}
	}

	if (fabs(NdotGCD) > 0.0f)
	{
		f32 lenGCD = floral::length(gCD);
		if (lenGCD > 0.0f)
		{
			f32 gammaCD = PId2 - atanf(floral::dot(PC, PD) / lenGCD);
			sum += NdotGCD * gammaCD / lenGCD;
		}
	}

	if (fabs(NdotGDA) > 0.0f)
	{
		f32 lenGDA = floral::length(gDA);
		if (lenGDA > 0.0f)
		{
			f32 gammaDA = PId2 - atanf(floral::dot(PD, PA) / lenGDA);
			sum += NdotGDA * gammaDA / lenGDA;
		}
	}

	sum *= PIt2inv;
	return sum;
}

const floral::vec3f get_random_point_on_quad(const floral::vec3f i_quad[])
{
	static std::random_device s_rd;
	static std::mt19937 s_gen(s_rd());
	static std::uniform_real_distribution<f32> s_dis(0.0f, 1.0f);

	floral::vec3f u = i_quad[1] - i_quad[0];
	floral::vec3f v = i_quad[3] - i_quad[0];
	floral::vec3f p = i_quad[0] + u * s_dis(s_gen) + v * s_dis(s_gen);
	return p;
}

const bool FormFactorsValidating::IsSegmentHitGeometry(const floral::vec3f& i_pi, const floral::vec3f& i_pj)
{
	f32 dist = floral::length(i_pj - i_pi);

	floral::ray3df r;
	r.o = i_pi;
	r.d = floral::normalize(i_pj - i_pi);

	for (size i = 0; i < m_RenderIndexData.get_size(); i += 3)
	{
		size vIdx0 = m_RenderIndexData[i];
		size vIdx1 = m_RenderIndexData[i + 1];
		size vIdx2 = m_RenderIndexData[i + 2];

		floral::vec3f p0 = m_RenderVertexData[vIdx0].Position;
		floral::vec3f p1 = m_RenderVertexData[vIdx1].Position;
		floral::vec3f p2 = m_RenderVertexData[vIdx2].Position;

		f32 t = 0.0f;
		const bool rh = floral::ray_triangle_intersect(r, p0, p1, p2, &t);
		if (rh)
		{
			if (t >= 0.001f && fabs(t - dist) > 0.001f)
			{
				return true;
			}
		}
	}
	return false;
}

void FormFactorsValidating::CalculateFormFactors()
{
	size patchCount = m_Patches.get_size();
	const s32 k_sampleCount = 8;
	for (size i = 0; i < patchCount; i++)
	//size i = 10;
	{
		for (size j = i + 1; j < patchCount; j++)
		//size j = 337;
		{
			f32 ff = 0.0f;
			for (s32 k = 0; k < k_sampleCount; k++)
			{
				floral::vec3f pi = get_random_point_on_quad(m_Patches[i].Vertex);
				f32 df = 0.0f, dg = 0.0f;
				for (s32 l = 0; l < k_sampleCount; l++)
				{
					f32 r = 0.0f;
					floral::vec3f pj, pij;
					while (r <= 0.001f)
					{
						pj = get_random_point_on_quad(m_Patches[j].Vertex);
						pij = pj - pi;
						r = floral::length(pij);
					}

					floral::vec3f nij = floral::normalize(pij);
					floral::vec3f nji = -nij;
					f32 vis = IsSegmentHitGeometry(pi, pj) ? 0.0f : 1.0f;
					f32 gVis = floral::dot(nij, m_Patches[i].Normal) * floral::dot(nji, m_Patches[j].Normal) / (3.141592f * r * r);
					if (gVis > 0.0f)
					{
						df += gVis * vis;
						dg += gVis;
					}
					FLORAL_ASSERT(df >= 0.0f);
					FLORAL_ASSERT(dg >= 0.0f);
				}

				if (df > 0.0f)
				{
					f32 gVisJ = compute_accurate_point2patch_form_factor(m_Patches[j].Vertex, pi, m_Patches[i].Normal);
					if (dg > 0.0f)
					{
						ff += (df / dg) *  (1.0f / k_sampleCount) * gVisJ;
					}
				}
			}

			if (ff > 0.0f)
			{
				CLOVER_ERROR("F[%zd; %zd] = %f", i, j, ff);
				ff = 0.0f;
			}

			m_FF[i][j] = fabs(ff);
			m_FF[j][i] = fabs(ff);
		}
	}
}

void FormFactorsValidating::CalculateRadiosity()
{
	for (size i = 0; i < m_Patches.get_size(); i++)
	{
		Patch& pi = m_Patches[i];
		for (size j = i + 1; j < m_Patches.get_size(); j++)
		{
			if (m_FF[i][j] > 0.0f)
			{
				Patch& pj = m_Patches[j];
				pi.RadiosityColor += m_FF[i][j] * pj.Color;
				pj.RadiosityColor += m_FF[i][j] * pi.Color;
			}
		}
		
		// poorman's software rasterizer :(
		ssize minX = 9999, minY = 9999;
		ssize maxX = -9999, maxY = -9999;
		for (size i = 0; i < 4; i++) 
		{
			if (pi.PixelCoord[i].x < minX) minX = pi.PixelCoord[i].x;
			if (pi.PixelCoord[i].x > maxX) maxX = pi.PixelCoord[i].x;
			if (pi.PixelCoord[i].y < minY) minY = pi.PixelCoord[i].y;
			if (pi.PixelCoord[i].y > maxY) maxY = pi.PixelCoord[i].y;
		}

		for (size i = minX; i < maxX; i++)
		{
			for (size j = minY; j < maxY; j++)
			{
				size pixelIdx = j * 512 + i;
				m_LightMapData[pixelIdx] = pi.RadiosityColor;
			}
		}
	}
}

}
