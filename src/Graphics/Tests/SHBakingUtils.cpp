#include "SHBakingUtils.h"

#include <clover.h>

#include <insigne/commons.h>
#include <insigne/counters.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include "Memory/MemorySystem.h"

namespace stone {

#define MAX_SH_ORDER							3
#define NUM_SH_COEFF							MAX_SH_ORDER * MAX_SH_ORDER
#define CP_PI									3.14159265359

static void BuildNormalizerHStrip(const s32 i_size, f32* o_output)
{
	s32 stripWidth = i_size * 6;
	//iterate over cube faces
	for(s32 iCubeFace = 0; iCubeFace < 6; iCubeFace++)
	{
		//fast texture walk, build normalizer cube map
		f32 *texelPtr = o_output;
		for(s32 v = 0; v < i_size; v++) // scanline
		{
			for(s32 u = 0; u < i_size; u++) // pixel
			{
				// note that the captured frame buffer image is flipped upside down, thus we dont need to invert the 
				// texture v coordinate
				floral::vec3f cubeCoord = floral::texel_coord_to_cube_coord(iCubeFace, (f32)u, (f32)v, i_size);
				texelPtr[(v * stripWidth + iCubeFace * i_size + u) * 4] = cubeCoord.x;
				texelPtr[(v * stripWidth + iCubeFace * i_size + u) * 4 + 1] = cubeCoord.y;
				texelPtr[(v * stripWidth + iCubeFace * i_size + u) * 4 + 2] = cubeCoord.z;
				texelPtr[(v * stripWidth + iCubeFace * i_size + u) * 4 + 3] =
					floral::texel_coord_to_solid_angle(iCubeFace, (f32)u, (f32)(i_size - 1 - v), i_size);
			}
		}
	}
}

static f64 SHBandFactor[NUM_SH_COEFF] =
{ 1.0,
2.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0,
1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0 };

static void EvalSHBasis(const f32* dir, f64* res )
{
	// Can be optimize by precomputing constant.
	static const f64 SqrtPi = sqrt(CP_PI);

	f64 xx = dir[0];
	f64 yy = dir[1];
	f64 zz = dir[2];

	// x[i] == pow(x, i), etc.
	f64 x[MAX_SH_ORDER+1], y[MAX_SH_ORDER+1], z[MAX_SH_ORDER+1];
	x[0] = y[0] = z[0] = 1.;
	for (s32 i = 1; i < MAX_SH_ORDER+1; ++i)
	{
		x[i] = xx * x[i-1];
		y[i] = yy * y[i-1];
		z[i] = zz * z[i-1];
	}

	res[0]  = (1/(2.*SqrtPi));

	res[1]  = -(sqrt(3/CP_PI)*yy)/2.;
	res[2]  = (sqrt(3/CP_PI)*zz)/2.;
	res[3]  = -(sqrt(3/CP_PI)*xx)/2.;

	res[4]  = (sqrt(15/CP_PI)*xx*yy)/2.;
	res[5]  = -(sqrt(15/CP_PI)*yy*zz)/2.;
	res[6]  = (sqrt(5/CP_PI)*(-1 + 3*z[2]))/4.;
	res[7]  = -(sqrt(15/CP_PI)*xx*zz)/2.;
	res[8]  = sqrt(15/CP_PI)*(x[2] - y[2])/4.;
}

void ComputeSH(f64* o_shr, f64* o_shg, f64* o_shb, f32* i_envMap)
{
	s32 width = 1536;
	s32 height = 256;
	// the stbi_loadf() will load the image with the order from top-down scanlines (y), left-right pixels (x)
	f32* data = i_envMap;
	s32 faceWidth = width / 6;
	s32 probesCount = height / faceWidth;

	f32* normalizeData = g_TemporalLinearArena.allocate_array<f32>(faceWidth * faceWidth * 4 * 6);
	BuildNormalizerHStrip(faceWidth, normalizeData);

	for (s32 pc = 0; pc < probesCount; pc++) {		
		f64 SHdir[NUM_SH_COEFF];
		memset(o_shr, 0, NUM_SH_COEFF * sizeof(f64));
		memset(o_shg, 0, NUM_SH_COEFF * sizeof(f64));
		memset(o_shb, 0, NUM_SH_COEFF * sizeof(f64));
		memset(SHdir, 0, NUM_SH_COEFF * sizeof(f64));

		f64 weightAccum = 0.0;
		f64 weight = 0.0;
		
		for (s32 iFaceIdx = 0; iFaceIdx < 6; iFaceIdx++)
		{
			for (s32 y = 0; y < faceWidth; y++) // scanline
			{
				s32 yy = y + pc * faceWidth;

				for (s32 x = 0; x < faceWidth; x++) // pixel
				{
					f32* texelVect = &normalizeData[(y * width + iFaceIdx * faceWidth + x) * 4];
					weight = *(texelVect + 3);

					EvalSHBasis(texelVect, SHdir);

					// Convert to f64
					f64 R = data[(yy * width + iFaceIdx * faceWidth + x) * 3];
					f64 G = data[(yy * width + iFaceIdx * faceWidth + x) * 3 + 1];
					f64 B = data[(yy * width + iFaceIdx * faceWidth + x) * 3 + 2];

					for (s32 i = 0; i < NUM_SH_COEFF; i++)
					{
						o_shr[i] += R * SHdir[i] * weight;
						o_shg[i] += G * SHdir[i] * weight;
						o_shb[i] += B * SHdir[i] * weight;
					}

					weightAccum += weight;
				}
			}
		}
		
		CLOVER_DEBUG("%d ---", pc);
		for (s32 i = 0; i < NUM_SH_COEFF; ++i)
		{
			o_shr[i] *= 4.0 * CP_PI / weightAccum;
			o_shg[i] *= 4.0 * CP_PI / weightAccum;
			o_shb[i] *= 4.0 * CP_PI / weightAccum;

			CLOVER_DEBUG("floral::vec4f(%ff, %ff, %ff, 0.0f);", o_shr[i], o_shg[i], o_shb[i]);
		}
	}
}

//----------------------------------------------

static const_cstr s_ProbeValidatorVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

out mediump vec4 v_VertexColor;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_VertexColor = l_Color;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_ProbeValidatorFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_VertexColor;

void main()
{
#if 1
	if (gl_FrontFacing)
	{
		o_Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		o_Color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}
#else
	o_Color = v_VertexColor;
#endif
}
)";

ProbeValidator::ProbeValidator()
{
}

ProbeValidator::~ProbeValidator()
{
}

void ProbeValidator::Initialize()
{
	// render buffer
	{
		// 1536 x 256
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		desc.color_attachments->push_back(insigne::color_attachment_t("main_color", insigne::texture_format_e::hdr_rgb));
		desc.width = 1536; desc.height = 256;
		m_ProbeRB = insigne::create_framebuffer(desc);
	}

	// scene data uniform buffer
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(512);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);
		m_ProbeSceneDataUB = newUB;
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_ProbeValidatorVS);
		strcpy(desc.fs, s_ProbeValidatorFS);
		desc.vs_path = floral::path("/scene/probe_validation_vs");
		desc.fs_path = floral::path("/scene/probe_validation_fs");

		m_ValidationShader = insigne::create_shader(desc);
		insigne::infuse_material(m_ValidationShader, m_ValidationMaterial);

		// static uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_ValidationMaterial, "ub_Scene");
			m_ValidationMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 256, m_ProbeSceneDataUB };
		}
	}
}

void ProbeValidator::Setup(const floral::mat4x4f& i_XForm, floral::fixed_array<floral::vec3f, LinearAllocator>& i_shPositions)
{
	m_ProbeLocs.init(i_shPositions.get_size(), &g_StreammingAllocator);
	m_ValidatedProbeLocs.init(i_shPositions.get_size(), &g_StreammingAllocator);
	m_ProbeSceneData.init(i_shPositions.get_size() * 6, &g_StreammingAllocator);

	m_ProbeLocs = i_shPositions;
	m_CurrentProbeIdx = 0;

	static floral::vec3f faceUpDirs[] = {
		floral::vec3f(0.0f, 1.0f, 0.0f),	// positive X
		floral::vec3f(0.0f, 1.0f, 0.0f),	// negative X

		floral::vec3f(1.0f, 0.0f, 0.0f),	// positive Y
		floral::vec3f(0.0f, 0.0f, -1.0f),	// negative Y

		floral::vec3f(0.0f, 1.0f, 0.0f),	// positive Z
		floral::vec3f(0.0f, 1.0f, 0.0f),	// negative Z
	};

	static floral::vec3f faceLookAtDirs[] = {
		floral::vec3f(1.0f, 0.0f, 0.0f),	// positive X
		floral::vec3f(-1.0f, 0.0f, 0.0f),	// negative X

		floral::vec3f(0.0f, 1.0f, 0.0f),	// positive Y
		floral::vec3f(0.0f, -1.0f, 0.0f),	// negative Y

		floral::vec3f(0.0f, 0.0f, 1.0f),	// positive Z
		floral::vec3f(0.0f, 0.0f, -1.0f),	// negative Z
	};

	floral::camera_persp_t camProj;
	camProj.near_plane = 0.01f; camProj.far_plane = 20.0f;
	camProj.fov = 90.0f;
	camProj.aspect_ratio = 1.0f;
	floral::mat4x4f proj = floral::construct_perspective(camProj);

	for (u32 i = 0; i < i_shPositions.get_size(); i++) {
		for (u32 f = 0; f < 6; f++) {
			floral::camera_view_t camView;
			camView.position = i_shPositions[i];
			camView.up_direction = faceUpDirs[f];
			camView.look_at = faceLookAtDirs[f];
			floral::mat4x4f wvp = proj * floral::construct_lookat_dir(camView);
			m_ProbeSceneData.push_back(SceneData{ i_XForm, wvp });
		}
	}

	g_TemporalLinearArena.free_all();
	p8 cpuData = (p8)g_TemporalLinearArena.allocate(SIZE_KB(512));
	memset(cpuData, 0, SIZE_KB(512));
	for (u32 i = 0; i < m_ProbeSceneData.get_size(); i++) {
		p8 pData = cpuData + 256 * i;
		memcpy(pData, &m_ProbeSceneData[i], sizeof(SceneData));
	}

	insigne::copy_update_ub(m_ProbeSceneDataUB, cpuData, 256 * m_ProbeSceneData.get_size(), 0);
}

void ProbeValidator::Validate(floral::simple_callback<void, const insigne::material_desc_t&> i_renderCb)
{
	if (!m_ValidationFinished) {
		if (!m_CurrentProbeCaptured) {
			for (u32 f = 0; f < 6; f++) {
				insigne::begin_render_pass(m_ProbeRB, 256 * f, 0, 256, 256);
				u32 ubSlot = insigne::get_material_uniform_block_slot(m_ValidationMaterial, "ub_Scene");
				m_ValidationMaterial.uniform_blocks[ubSlot].value.offset = 256 * (f + 6 * m_CurrentProbeIdx);

				i_renderCb(m_ValidationMaterial);

				insigne::end_render_pass(m_ProbeRB);
				insigne::dispatch_render_pass();
			}

			g_TemporalLinearArena.free_all();
			m_ProbePixelData = g_TemporalLinearArena.allocate_array<f32>(1536 * 256 * 3);
			m_PixelDataReadyFrameIdx = insigne::schedule_framebuffer_capture(m_ProbeRB, m_ProbePixelData);
			m_CurrentProbeCaptured = true;
		}

		if (m_CurrentProbeCaptured && insigne::get_current_frame_idx() >= m_PixelDataReadyFrameIdx) {
			f32 total = 0.0f;
			for (size i = 0; i < 1536 * 256; i++)
			{
				total += m_ProbePixelData[i * 3];
			}
			total = total / (1536.0f * 256.0f) * 100.0f;
			if (total <= 10.0f)
			{
				CLOVER_DEBUG("Capture probe %d: %4.2f - REJECTED", m_CurrentProbeIdx, total);
			}
			else
			{
				CLOVER_DEBUG("Capture probe %d: %4.2f", m_CurrentProbeIdx, total);
				m_ValidatedProbeLocs.push_back(m_ProbeLocs[m_CurrentProbeIdx]);
			}
			m_CurrentProbeCaptured = false;
			m_CurrentProbeIdx++;
		}
		if (m_CurrentProbeIdx == m_ProbeLocs.get_size()) m_ValidationFinished = true;
		//if (m_CurrentProbeIdx == 186) m_ValidationFinished = true;
	}
}

//----------------------------------------------

SHBaker::SHBaker()
{
}

SHBaker::~SHBaker()
{
}

void SHBaker::Initialize(const floral::mat4x4f& i_XForm, floral::fixed_array<floral::vec3f, LinearAllocator>& i_shPositions)
{
	m_SHPositions.init(i_shPositions.get_size(), &g_StreammingAllocator);
	m_EnvSceneData.init(i_shPositions.get_size() * 6, &g_StreammingAllocator);

	m_SHPositions = i_shPositions;

	static floral::vec3f faceUpDirs[] = {
		floral::vec3f(0.0f, 1.0f, 0.0f),	// positive X
		floral::vec3f(0.0f, 1.0f, 0.0f),	// negative X

		floral::vec3f(1.0f, 0.0f, 0.0f),	// positive Y
		floral::vec3f(0.0f, 0.0f, -1.0f),	// negative Y

		floral::vec3f(0.0f, 1.0f, 0.0f),	// positive Z
		floral::vec3f(0.0f, 1.0f, 0.0f),	// negative Z
	};

	static floral::vec3f faceLookAtDirs[] = {
		floral::vec3f(1.0f, 0.0f, 0.0f),	// positive X
		floral::vec3f(-1.0f, 0.0f, 0.0f),	// negative X

		floral::vec3f(0.0f, 1.0f, 0.0f),	// positive Y
		floral::vec3f(0.0f, -1.0f, 0.0f),	// negative Y

		floral::vec3f(0.0f, 0.0f, 1.0f),	// positive Z
		floral::vec3f(0.0f, 0.0f, -1.0f),	// negative Z
	};

	floral::camera_persp_t camProj;
	camProj.near_plane = 0.01f; camProj.far_plane = 20.0f;
	camProj.fov = 90.0f;
	camProj.aspect_ratio = 1.0f;
	floral::mat4x4f proj = floral::construct_perspective(camProj);

	for (u32 i = 0; i < i_shPositions.get_size(); i++) {
		for (u32 f = 0; f < 6; f++) {
			floral::camera_view_t camView;
			camView.position = i_shPositions[i] + floral::vec3f(0.0f, 0.2f, 0.0f);
			camView.up_direction = faceUpDirs[f];
			camView.look_at = faceLookAtDirs[f];
			floral::mat4x4f wvp = proj * floral::construct_lookat_dir(camView);
			m_EnvSceneData.push_back(SceneData{ i_XForm, wvp });
		}
	}

	{
		// 1536 x 256
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		desc.color_attachments->push_back(insigne::color_attachment_t("main_color", insigne::texture_format_e::hdr_rgb));
		desc.width = 1536; desc.height = 256;
		m_EnvMapRenderBuffer = insigne::create_framebuffer(desc);
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(512);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		g_TemporalLinearArena.free_all();
		FLORAL_ASSERT(256 * m_EnvSceneData.get_size() <= desc.region_size);
		p8 cpuData = (p8)g_TemporalLinearArena.allocate(desc.region_size);
		memset(cpuData, 0, desc.region_size);

		for (u32 i = 0; i < m_EnvSceneData.get_size(); i++) {
			p8 pData = cpuData + 256 * i;
			memcpy(pData, &m_EnvSceneData[i], sizeof(SceneData));
		}

		insigne::copy_update_ub(newUB, cpuData, 256 * m_EnvSceneData.get_size(), 0);
		m_EnvMapSceneUB = newUB;
	}

	//insigne::dispatch_render_pass();
}

void SHBaker::SetupValidator(insigne::material_desc_t& io_material)
{
	m_FinishValidation = false;
	m_EnvCaptured = false;
	m_FrameIdx = 0;
	m_EnvIdx = 185;
	//m_EnvIdx = 0;
	{
		u32 ubSlot = insigne::get_material_uniform_block_slot(io_material, "ub_Scene");
		io_material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 256, m_EnvMapSceneUB };
	}
}

void SHBaker::Validate(insigne::material_desc_t& i_material, floral::simple_callback<void, const bool> i_renderCb)
{
	if (!m_FinishValidation) {
		if (!m_EnvCaptured) {
			for (u32 f = 0; f < 6; f++) {
				insigne::begin_render_pass(m_EnvMapRenderBuffer, 256 * f, 0, 256, 256);
				u32 ubSlot = insigne::get_material_uniform_block_slot(i_material, "ub_Scene");
				i_material.uniform_blocks[ubSlot].value.offset = 256 * (f + 6 * m_EnvIdx);

				//insigne::draw_surface<SurfacePNC>(m_VB, m_IB, i_material);
				i_renderCb(true);

				insigne::end_render_pass(m_EnvMapRenderBuffer);
				insigne::dispatch_render_pass();
			}

			g_TemporalLinearArena.free_all();
			m_EnvMapPixelData = g_TemporalLinearArena.allocate_array<f32>(1536 * 256 * 3);
			m_FrameIdx = insigne::schedule_framebuffer_capture(m_EnvMapRenderBuffer, m_EnvMapPixelData);
			m_EnvCaptured = true;
		}

		if (m_EnvCaptured && insigne::get_current_frame_idx() >= m_FrameIdx) {
			m_EnvIdx++;
			m_EnvCaptured = false;
			f32 total = 0.0f;
			for (size i = 0; i < 1536 * 256; i++)
			{
				total += m_EnvMapPixelData[i * 3];
			}
			total = total / (1536.0f * 256.0f) * 100.0f;
			if (total <= 10.0f)
			{
				CLOVER_DEBUG("Capture %d probes: %4.2f - REJECTED", m_EnvIdx, total);
			}
			else
			{
				CLOVER_DEBUG("Capture %d probes: %4.2f", m_EnvIdx, total);
			}
		}
		//if (m_EnvIdx == m_SHPositions.get_size()) m_FinishValidation = true;
		if (m_EnvIdx == 186) m_FinishValidation = true;
	}
}

}
