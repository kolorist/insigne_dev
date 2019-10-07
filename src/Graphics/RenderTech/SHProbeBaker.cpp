#include "SHProbeBaker.h"

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/counters.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

namespace stone
{
namespace tech
{

constexpr static size k_MaxSHOrder				= 3;
constexpr static size k_NumSHCoEffs				= k_MaxSHOrder * k_MaxSHOrder;

//----------------------------------------------

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

static f64 SHBandFactor[k_NumSHCoEffs] =
{ 1.0,
2.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0,
1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0 };

static void EvalSHBasis(const f32* dir, f64* res )
{
	// Can be optimize by precomputing constant.
	static const f64 SqrtPi = sqrt(floral::pi);

	f64 xx = dir[0];
	f64 yy = dir[1];
	f64 zz = dir[2];

	// x[i] == pow(x, i), etc.
	f64 x[k_MaxSHOrder+1], y[k_MaxSHOrder+1], z[k_MaxSHOrder+1];
	x[0] = y[0] = z[0] = 1.;
	for (s32 i = 1; i < k_MaxSHOrder+1; ++i)
	{
		x[i] = xx * x[i-1];
		y[i] = yy * y[i-1];
		z[i] = zz * z[i-1];
	}

	res[0]  = (1/(2.*SqrtPi));

	res[1]  = -(sqrt(3/floral::pi)*yy)/2.;
	res[2]  = (sqrt(3/floral::pi)*zz)/2.;
	res[3]  = -(sqrt(3/floral::pi)*xx)/2.;

	res[4]  = (sqrt(15/floral::pi)*xx*yy)/2.;
	res[5]  = -(sqrt(15/floral::pi)*yy*zz)/2.;
	res[6]  = (sqrt(5/floral::pi)*(-1 + 3*z[2]))/4.;
	res[7]  = -(sqrt(15/floral::pi)*xx*zz)/2.;
	res[8]  = sqrt(15/floral::pi)*(x[2] - y[2])/4.;
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

	for (s32 pc = 0; pc < probesCount; pc++)
	{		
		f64 SHdir[k_NumSHCoEffs];
		memset(o_shr, 0, k_NumSHCoEffs * sizeof(f64));
		memset(o_shg, 0, k_NumSHCoEffs * sizeof(f64));
		memset(o_shb, 0, k_NumSHCoEffs * sizeof(f64));
		memset(SHdir, 0, k_NumSHCoEffs * sizeof(f64));

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

					for (s32 i = 0; i < k_NumSHCoEffs; i++)
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
		for (s32 i = 0; i < k_NumSHCoEffs; ++i)
		{
			o_shr[i] *= 4.0 * floral::pi / weightAccum;
			o_shg[i] *= 4.0 * floral::pi / weightAccum;
			o_shb[i] *= 4.0 * floral::pi / weightAccum;

			CLOVER_DEBUG("floral::vec4f(%ff, %ff, %ff, 0.0f);", o_shr[i], o_shg[i], o_shb[i]);
		}
	}
	g_TemporalLinearArena.free(normalizeData);
}


//----------------------------------------------

SHProbeBaker::SHProbeBaker()
	: m_ResourceArena(nullptr)
	, m_ProbePixelData(nullptr)
	, m_CurrentProbeIdx(0)
	, m_PixelDataReadyFrameIdx(0)
	, m_CurrentProbeCaptured(false)
	, m_IsSHBakingFinished(true)
{
}

SHProbeBaker::~SHProbeBaker()
{
}

void SHProbeBaker::Initialize()
{
	m_ResourceArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(8));

	// render buffer
	{
		// 1536 x 256
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		desc.color_attachments->push_back(insigne::color_attachment_t("main_color", insigne::texture_format_e::hdr_rgb));
		desc.width = 1536; desc.height = 256;
		m_ProbeRB = insigne::create_framebuffer(desc);
	}

	m_ProbePixelData = m_ResourceArena->allocate_array<f32>(1536 * 256 * 3);
}

void SHProbeBaker::CleanUp()
{
	g_PersistanceResourceAllocator.free(m_ResourceArena);
	m_ResourceArena = nullptr;
	m_ProbePixelData = nullptr;
	m_IsSHBakingFinished = true;
	m_CurrentProbeIdx = 0;
}

const bool SHProbeBaker::FrameUpdate()
{
	if (m_IsSHBakingFinished)
	{
		return true;
	}

	if (!m_CurrentProbeCaptured)
	{
		const floral::vec3f& currProbePos = m_SHPositions[m_CurrentProbeIdx];
		for (u32 f = 0; f < 6; f++)
		{
			insigne::begin_render_pass(m_ProbeRB, 256 * f, 0, 256, 256);

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

			floral::camera_view_t camView;
			camView.position = currProbePos;
			camView.up_direction = faceUpDirs[f];
			camView.look_at = faceLookAtDirs[f];
			floral::mat4x4f wvp = proj * floral::construct_lookat_dir(camView);
			m_RenderCB(wvp);

			insigne::end_render_pass(m_ProbeRB);
			insigne::dispatch_render_pass();
		}

		m_PixelDataReadyFrameIdx = insigne::schedule_framebuffer_capture(m_ProbeRB, m_ProbePixelData);
		m_CurrentProbeCaptured = true;
	}

	if (m_CurrentProbeCaptured && insigne::get_current_frame_idx() >= m_PixelDataReadyFrameIdx)
	{
		f64 shr[9];
		f64 shg[9];
		f64 shb[9];
		ComputeSH(shr, shg, shb, m_ProbePixelData);
		SHData shData;
		for (u32 i = 0; i < 9; i++)
		{
			shData.CoEffs[i].x = (f32)shr[i];
			shData.CoEffs[i].y = (f32)shg[i];
			shData.CoEffs[i].z = (f32)shb[i];
			shData.CoEffs[i].w = 0.0f;
		}
		m_SHOutputBuffer[m_CurrentProbeIdx] = shData;

		m_CurrentProbeCaptured = false;
		m_CurrentProbeIdx++;
		CLOVER_DEBUG("SH captured: %d", m_CurrentProbeIdx);
	}

	if (m_CurrentProbeIdx == m_SHPositions.get_size())
	{
		m_IsSHBakingFinished = true;
		return true;
	}

	return false;
}

}
}
