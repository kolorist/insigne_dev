#include "AccurateFormFactor.h"
#include <calyx/context.h>

#include <clover.h>

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include <random>

namespace stone
{

//----------------------------------------------

#define PId2     1.570796326794896619f   /* pi / 2 */
#define PIt2inv  0.159154943091895346f   /* 1 / (2 * pi) */

/*
 * returns
 *  - positive IF i_quad vertices are declared in clockwise order
 *  - negative IF i_quad vertices are declared in counter-clockwise order
 */
static const f32 compute_accurate_point2patch_form_factor(
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

#define NUM_SAMPLE_PER_PATCH					16u

struct compute_env_t
{
};

static const floral::vec3f get_random_point_on_quad(const floral::vec3f i_quad[])
{
	static std::random_device s_rd;
	static std::mt19937 s_gen(s_rd());
	static std::uniform_real_distribution<f32> s_dis(0.0f, 1.0f);

	floral::vec3f u = i_quad[1] - i_quad[0];
	floral::vec3f v = i_quad[3] - i_quad[0];
	floral::vec3f p = i_quad[0] + u * s_dis(s_gen) + v * s_dis(s_gen);
	return p;
}

static const f32 compute_patch2patch_form_factor(
	const floral::vec3f i_srcQuad[], const floral::vec3f& i_srcNorm,
	const floral::vec3f i_dstQuad[], const floral::vec3f& i_dstNorm,
	compute_env_t* io_env)
{
	f32 sum = 0.0f;

	// generate sample points
	for (s32 i = 0; i < NUM_SAMPLE_PER_PATCH; i++)
	{
		floral::vec3f pi = get_random_point_on_quad(i_srcQuad);
		f32 df = 0.0f, dg = 0.0f;
		for (s32 j = 0; j < NUM_SAMPLE_PER_PATCH; j++)
		{
			f32 r = 0.0f;
			floral::vec3f pj, pij;
			while (r <= 0.001f)
			{
				pj = get_random_point_on_quad(i_dstQuad);
				pij = pj - pi;
				r = floral::length(pij);
			}

			floral::vec3f nij = floral::normalize(pij);
			floral::vec3f nji = -nij;
			f32 vis = 1.0f; // hit other geometry? no for now
			f32 gVis = floral::dot(nij, i_srcNorm) * floral::dot(nji, i_dstNorm) / (3.141592f * r * r);

			if (gVis > 0.0f)
			{
				df += gVis * vis;
				dg += gVis;
			}
		}

		f32 gVisJ = compute_accurate_point2patch_form_factor(i_dstQuad, pi, i_srcNorm);
		if (gVisJ > 0.0f)
		{
			sum += (df / dg) *  (1.0f / NUM_SAMPLE_PER_PATCH) * gVisJ;
		}
	}

	return sum;
}

//----------------------------------------------

AccurateFormFactor::AccurateFormFactor()
	: m_CameraMotion(
			floral::camera_view_t { floral::vec3f(3.0f, 3.0f, -3.0f), floral::vec3f(-3.0f, -3.0f, 3.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
			floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_SrcPatchRY(0.0f)
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

AccurateFormFactor::~AccurateFormFactor()
{
}

void AccurateFormFactor::OnInitialize()
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

	const f32 k_scale = 0.4f;

	m_SrcPatch.Vertex[0] = floral::vec3f(0.5f, 0.5f, 0.0f) * k_scale;
	m_SrcPatch.Vertex[1] = floral::vec3f(0.5f, -0.5f, 0.0f) * k_scale;
	m_SrcPatch.Vertex[2] = floral::vec3f(-0.5f, -0.5f, 0.0f) * k_scale;
	m_SrcPatch.Vertex[3] = floral::vec3f(-0.5f, 0.5f, 0.0f) * k_scale;
	m_SrcPatch.Normal = floral::vec3f(0.0f, 0.0f, 1.0f);

	m_OrgSrcPatch = m_SrcPatch;

	m_DstPatch.Vertex[0] = floral::vec3f(0.0f, 0.5f, 0.5f) * k_scale + floral::vec3f(1.0f, 0.0f, 0.0f);
	m_DstPatch.Vertex[1] = floral::vec3f(0.0f, -0.5f, 0.5f) * k_scale + floral::vec3f(1.0f, 0.0f, 0.0f);
	m_DstPatch.Vertex[2] = floral::vec3f(0.0f, -0.5f, -0.5f) * k_scale + floral::vec3f(1.0f, 0.0f, 0.0f);
	m_DstPatch.Vertex[3] = floral::vec3f(0.0f, 0.5f, -0.5f) * k_scale + floral::vec3f(1.0f, 0.0f, 0.0f);
	m_DstPatch.Normal = floral::vec3f(-1.0f, 0.0f, 0.0f);

	m_DebugDrawer.Initialize();
}

void AccurateFormFactor::OnUpdate(const f32 i_deltaMs)
{
	static f32 s_elapsedTime = 0.0f;
	s_elapsedTime += i_deltaMs;

	m_CameraMotion.OnUpdate(i_deltaMs);

	floral::quaternionf q =
		floral::construct_quaternion_euler(floral::vec3f(0.0f, m_SrcPatchRY, 0.0f));
	floral::mat4x4f m = q.to_transform();

	m_SrcPatch.Vertex[0] = floral::apply_point_transform(m_OrgSrcPatch.Vertex[0], m);
	m_SrcPatch.Vertex[1] = floral::apply_point_transform(m_OrgSrcPatch.Vertex[1], m);
	m_SrcPatch.Vertex[2] = floral::apply_point_transform(m_OrgSrcPatch.Vertex[2], m);
	m_SrcPatch.Vertex[3] = floral::apply_point_transform(m_OrgSrcPatch.Vertex[3], m);
	m_SrcPatch.Normal = floral::apply_vector_transform(m_OrgSrcPatch.Normal, m);

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

	m_DebugDrawer.DrawQuad3D(m_SrcPatch.Vertex[0], m_SrcPatch.Vertex[1],
			m_SrcPatch.Vertex[2], m_SrcPatch.Vertex[3], floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawQuad3D(m_DstPatch.Vertex[0], m_DstPatch.Vertex[1],
			m_DstPatch.Vertex[2], m_DstPatch.Vertex[3], floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.0f, 0.0f, 0.0f), m_SrcPatch.Normal * 0.3f, floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));

	// gvisJ calculation draw
	floral::vec3f dstP = (m_DstPatch.Vertex[0] + m_DstPatch.Vertex[2]) / 2.0f;
	m_DebugDrawer.DrawLine3D(dstP, m_SrcPatch.Vertex[0], floral::vec4f(0.2f, 0.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(dstP, m_SrcPatch.Vertex[1], floral::vec4f(0.4f, 0.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(dstP, m_SrcPatch.Vertex[2], floral::vec4f(0.8f, 0.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(dstP, m_SrcPatch.Vertex[3], floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));

	m_DebugDrawer.EndFrame();
}

void AccurateFormFactor::OnDebugUIUpdate(const f32 i_deltaMs)
{
	calyx::context_attribs* commonCtx = calyx::get_context_attribs();

	static f32 gvisValues[180] = { 0 };
	static ssize valOffset = 0;

	ImGui::Begin("AccurateFormFactor Controller");

	floral::vec3f dstP = (m_DstPatch.Vertex[0] + m_DstPatch.Vertex[2]) / 2.0f;
	f32 gVisJ = compute_accurate_point2patch_form_factor(
			m_SrcPatch.Vertex, dstP, m_DstPatch.Normal);

	f32 ff = compute_patch2patch_form_factor(
			m_SrcPatch.Vertex, m_SrcPatch.Normal,
			m_DstPatch.Vertex, m_DstPatch.Normal,
			nullptr);

	if (ImGui::DragFloat("SrcPath rotationY", &m_SrcPatchRY, 1.0f, 0.0f, 360.0f))
	{
		gvisValues[valOffset] = gVisJ;
		valOffset = (valOffset + 1) % 90;
	}

	ImGui::Text("GVisJ: %f", gVisJ);
	ImGui::Text("ff: %f", ff);
	ImGui::PlotLines("Plotted GVisJ", gvisValues, 90, valOffset, nullptr, FLT_MAX, FLT_MAX, ImVec2(0, 80));

	//ImGui::ShowDemoWindow();

	ImGui::End();
}

void AccurateFormFactor::OnRender(const f32 i_deltaMs)
{
	// camera
	m_SceneData.WVP = m_CameraMotion.GetWVP();
	insigne::update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	m_DebugDrawer.Render(m_SceneData.WVP);
	IDebugUI::OnFrameRender(i_deltaMs);

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void AccurateFormFactor::OnCleanUp()
{
}

}
