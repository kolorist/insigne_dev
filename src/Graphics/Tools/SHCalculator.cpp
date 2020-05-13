#include "SHCalculator.h"

#include <math.h>

#include <floral/containers/fast_array.h>
#include <floral/comgeo/shapegen.h>
#include <floral/math/transform.h>
#include <floral/io/nativeio.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/counters.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include <calyx/context.h>

#include "InsigneImGui.h"
#include "Graphics/stb_image.h"
#include "Graphics/stb_image_resize.h"
#include "Graphics/stb_image_write.h"
#include "Graphics/stb_dxt.h"
#include "Graphics/prt.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/CBTexDefinitions.h"
#include "Graphics/MaterialParser.h"
#include "Graphics/TextureLoader.h"

namespace stone
{
namespace tools
{
//--------------------------------------------------------------------

static const_cstr k_ProjectionSchemeStr[] = {
	"light probe",
	"h strip",
	"equirectangular",
	"invalid"
};

static const_cstr k_Images[] = {
	"<none>",
	"uvchecker_equi.hdr",
	"circus_arena_probe.hdr",

	"uvchecker_hstrip.hdr",
	"uvchecker_pos_x_hstrip.hdr",
	"uvchecker_neg_x_hstrip.hdr",
	"uvchecker_pos_y_hstrip.hdr",
	"uvchecker_neg_y_hstrip.hdr",
	"uvchecker_pos_z_hstrip.hdr",
	"uvchecker_neg_z_hstrip.hdr",

	"autumn_hockey_hstrip.hdr",
	"circus_arena_hstrip.hdr",
	"palermo_sidewalk_hstrip.hdr",
	"pond_bridge_night_hstrip.hdr",
	"small_cave_hstrip.hdr",
	"snowy_park_01_hstrip.hdr",
	"sunflowers_hstrip.hdr",
	"teatro_massimo_hstrip.hdr",
	"venice_sunset_hstrip.hdr",
	"viale_giuseppe_garibaldi_hstrip.hdr",
	"cloudy_vondelpark_hstrip.hdr",
	"papermill_ruin_hstrip.hdr",
};

static const floral::vec3f s_lookAts[] =
{
	floral::vec3f(1.0f, 0.0f, 0.0f),
	floral::vec3f(-1.0f, 0.0f, 0.0f),
	floral::vec3f(0.0f, 1.0f, 0.0f),
	floral::vec3f(0.0f, -1.0f, 0.0f),
	floral::vec3f(0.0f, 0.0f, 1.0f),
	floral::vec3f(0.0f, 0.0f, -1.0f)
};

static const floral::vec3f s_upDirs[] =
{
	floral::vec3f(0.0f, -1.0f, 0.0f),
	floral::vec3f(0.0f, -1.0f, 0.0f),
	floral::vec3f(0.0f, 0.0f, 1.0f),
	floral::vec3f(0.0f, 0.0f, -1.0f),
	floral::vec3f(0.0f, -1.0f, 0.0f),
	floral::vec3f(0.0f, -1.0f, 0.0f)
};

static const insigne::cubemap_face_e s_faces[] =
{
	insigne::cubemap_face_e::positive_x,
	insigne::cubemap_face_e::negative_x,
	insigne::cubemap_face_e::positive_y,
	insigne::cubemap_face_e::negative_y,
	insigne::cubemap_face_e::positive_z,
	insigne::cubemap_face_e::negative_z
};

static constexpr s32 k_faceSize = 512;
static constexpr s32 k_previewFaceSize = 100;

//--------------------------------------------------------------------

const ProjectionScheme get_projection_from_size(const s32 i_width, const s32 i_height)
{
	const f32 aspectRatio = (f32)i_width / (f32)i_height;
	if (i_width == i_height)
	{
		FLORAL_ASSERT(i_width == k_faceSize * 2);
		return ProjectionScheme::LightProbe;
	}
	else if (i_height * 6 == i_width)
	{
		FLORAL_ASSERT(i_height == k_faceSize);
		return ProjectionScheme::HStrip;
	}
	else if (fabs(aspectRatio - 2.0) < 0.0001)
	{
		FLORAL_ASSERT(i_height == k_faceSize * 2);
		return ProjectionScheme::Equirectangular;
	}
	return ProjectionScheme::Invalid;
}

//--------------------------------------------------------------------

void trim_image(f32* i_imgData, f32* o_outData, const s32 i_x, const s32 i_y, const s32 i_width, const s32 i_height,
		const s32 i_imgWidth, const s32 i_imgHeight, const s32 i_channelCount)
{
	s32 outScanline = 0;
	for (s32 i = i_y; i < (i_y + i_height); i++)
	{
		memcpy(&o_outData[outScanline * i_width * i_channelCount],
				&i_imgData[(i * i_imgWidth + i_x) * i_channelCount],
				i_width * i_channelCount * sizeof(f32));
		outScanline++;
	}
}

//--------------------------------------------------------------------

floral::vec3f hdr_tonemap(const f32* i_hdrRGB)
{
	floral::vec3f hdrColor(i_hdrRGB[0], i_hdrRGB[1], i_hdrRGB[2]);
	f32 maxLuma = floral::max(hdrColor.x, floral::max(hdrColor.y, hdrColor.z));
	hdrColor.x = hdrColor.x / (1.0f + maxLuma / 35.0f);
	hdrColor.y = hdrColor.y / (1.0f + maxLuma / 35.0f);
	hdrColor.z = hdrColor.z / (1.0f + maxLuma / 35.0f);
	return hdrColor;
}

//--------------------------------------------------------------------

floral::vec4f rgbm_encode(const floral::vec3f& i_hdrColor)
{
	floral::vec4f rgbm;
	floral::vec3f color = i_hdrColor / 6.0f;
	rgbm.w = floral::clamp(floral::max(floral::max(color.x, color.y), floral::max(color.z, 0.000001f)), 0.0f, 1.0f);
	rgbm.w = ceil(rgbm.w * 255.0f) / 255.0f;
	rgbm.x = color.x / rgbm.w;
	rgbm.y = color.y / rgbm.w;
	rgbm.z = color.z / rgbm.w;
	return rgbm;
}

//--------------------------------------------------------------------

void convert_to_rgbm(f32* i_inpData, p8 o_rgbmData, const s32 i_width, const s32 i_height, const f32 i_gamma)
{
	for (s32 y = 0; y < i_height; y++)
	{
		for (s32 x = 0; x < i_width; x++)
		{
			s32 pixelIdx = y * i_width + x;
			floral::vec3f hdrColor(i_inpData[pixelIdx * 3], i_inpData[pixelIdx * 3 + 1], i_inpData[pixelIdx * 3 + 2]);

			if (hdrColor.x < 0.0f || hdrColor.y < 0.0f || hdrColor.z < 0.0f)
			{
				CLOVER_WARNING("clamping pixel at (x, y) = (%d, %d) because it has negative component: (%f, %f, %f)", x, y, hdrColor.x, hdrColor.y, hdrColor.z);
				hdrColor.x = floral::max(hdrColor.x, 0.0f);
				hdrColor.y = floral::max(hdrColor.y, 0.0f);
				hdrColor.z = floral::max(hdrColor.z, 0.0f);
			}

			floral::vec3f gammaCorrectedHDRColor;
			gammaCorrectedHDRColor.x = powf(hdrColor.x, i_gamma);
			gammaCorrectedHDRColor.y = powf(hdrColor.y, i_gamma);
			gammaCorrectedHDRColor.z = powf(hdrColor.z, i_gamma);
			floral::vec4f rgbmFloatColor = rgbm_encode(gammaCorrectedHDRColor);
			FLORAL_ASSERT(rgbmFloatColor.x <= 1.0f);
			FLORAL_ASSERT(rgbmFloatColor.y <= 1.0f);
			FLORAL_ASSERT(rgbmFloatColor.z <= 1.0f);
			FLORAL_ASSERT(rgbmFloatColor.w <= 1.0f);
			o_rgbmData[pixelIdx * 4] = ceil(rgbmFloatColor.x * 255.0f);
			o_rgbmData[pixelIdx * 4 + 1] = ceil(rgbmFloatColor.y * 255.0f);
			o_rgbmData[pixelIdx * 4 + 2] = ceil(rgbmFloatColor.z * 255.0f);
			o_rgbmData[pixelIdx * 4 + 3] = ceil(rgbmFloatColor.w * 255.0f);
		}
	}
}

//--------------------------------------------------------------------

template <class TAllocator>
p8 compress_dxt(p8 i_input, const s32 i_width, const s32 i_height, const s32 i_numChannels, size* o_compressedSize, TAllocator* i_allocator)
{
	// 1 block = 4x4 pixels (with uncompressed size of 64 bytes for rgba textures)
	size bytesPerBlock = 0;
	size compressedSize = 0;
	const size pixelsCount = i_width * i_height;

	if (i_numChannels == 4)
	{
		// dxt5
		bytesPerBlock = 16;
		compressedSize = floral::max(pixelsCount, 16ull);
	}
	else
	{
		// dxt1 (only 1 bit for alpha)
		bytesPerBlock = 8;
		compressedSize = floral::max(pixelsCount / 2, 8ull);
	}

	p8 output = (p8)i_allocator->allocate(compressedSize);
	p8 targetBlock = output;

	for (s32 y = 0; y < i_height; y += 4)
	{
		for (s32 x = 0; x < i_width; x += 4)
		{
			u8 rawRGBA[4 * 4 * 4];
			memset(rawRGBA, 0, sizeof(rawRGBA));
			p8 targetPixel = rawRGBA;
			for (s32 py = 0; py < 4; py++)
			{
				for (s32 px = 0; px < 4; px++)
				{
					size ix = x + px;
					size iy = y + py;

					if (ix < i_width && iy < i_height)
					{
						p8 sourcePixel = &i_input[(iy * i_width + ix) * i_numChannels];
						// initialize the alpha channel for this pixel
						targetPixel[3] = 255;
						for (s32 i = 0; i < i_numChannels; i++)
						{
							targetPixel[i] = sourcePixel[i];
						}
						targetPixel += 4;
					}
					else
					{
						targetPixel += 4;
					}
				}
			}

			if (i_numChannels == 4)
			{
				stb_compress_dxt_block(targetBlock, rawRGBA, 1, STB_DXT_HIGHQUAL);
			}
			else
			{
				stb_compress_dxt_block(targetBlock, rawRGBA, 0, STB_DXT_HIGHQUAL);
			}

			targetBlock += bytesPerBlock;
		}
	}

	*(o_compressedSize) = compressedSize;
	return output;
}

//--------------------------------------------------------------------

SHCalculator::SHCalculator()
	: m_CameraMotion(
		floral::camera_view_t { floral::vec3f(6.0f, 0.0f, 0.0f), floral::vec3f(0.0f, 0.0f, 0.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
		floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_ImgLoaded(false)

	, m_ComputingSH(false)
	, m_SHReady(false)
	, m_Counter(0)

	, m_NeedBakePMREM(false)
	, m_PMREMReady(false)
	, m_IsCapturingPMREMData(false)
	, m_TexFileWritten(false)
{
}

//--------------------------------------------------------------------

SHCalculator::~SHCalculator()
{
}

//--------------------------------------------------------------------

ICameraMotion* SHCalculator::GetCameraMotion()
{
	return &m_CameraMotion;
}

//--------------------------------------------------------------------

const_cstr SHCalculator::GetName() const
{
	return k_name;
}

//--------------------------------------------------------------------

void SHCalculator::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	calyx::context_attribs* commonCtx = calyx::get_context_attribs();

	// register surfaces
	insigne::register_surface_type<geo3d::SurfaceP>();
	insigne::register_surface_type<geo2d::SurfacePT>();

	// memory arena
	m_TemporalArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(48));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(1));
	m_PostFXArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));

	// ico sphere
	floral::fast_fixed_array<geo3d::VertexP, FreelistArena> vtxData;
	floral::fast_fixed_array<s32, FreelistArena> idxData;

	vtxData.reserve(4096, m_TemporalArena);
	idxData.reserve(8192, m_TemporalArena);
	vtxData.resize(4096);
	idxData.resize(8192);

	floral::reset_generation_transforms_stack();
	floral::geo_generate_result_t genResult = floral::generate_unit_icosphere_3d(
			3, 0, sizeof(geo3d::VertexP),
			floral::geo_vertex_format_e::position,
			&vtxData[0], &idxData[0]);
	m_Sphere = helpers::CreateSurfaceGPU(&vtxData[0], genResult.vertices_generated, sizeof(geo3d::VertexP),
			&idxData[0], genResult.indices_generated, insigne::buffer_usage_e::static_draw, true);

	floral::reset_generation_transforms_stack();
	genResult = floral::generate_unit_icosphere_3d(
			2, 0, sizeof(geo3d::VertexP),
			floral::geo_vertex_format_e::position,
			&vtxData[0], &idxData[0]);
	m_Skysphere = helpers::CreateSurfaceGPU(&vtxData[0], genResult.vertices_generated, sizeof(geo3d::VertexP),
			&idxData[0], genResult.indices_generated, insigne::buffer_usage_e::static_draw, true);

	floral::reset_generation_transforms_stack();
	genResult = floral::generate_unit_box_3d(
			0, sizeof(geo3d::VertexP),
			floral::geo_vertex_format_e::position,
			&vtxData[0], &idxData[0]);

	m_PMREMSkybox = helpers::CreateSurfaceGPU(&vtxData[0], genResult.vertices_generated, sizeof(geo3d::VertexP),
			&idxData[0], genResult.indices_generated, insigne::buffer_usage_e::static_draw, true);

	{
		insigne::ubdesc_t desc;
		desc.region_size = 512;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		m_UB = insigne::create_ub(desc);

		m_CameraMotion.SetScreenResolution(commonCtx->window_width, commonCtx->window_height);

		m_SceneData.viewProjectionMatrix = m_CameraMotion.GetWVP();
		m_SceneData.cameraPosition = floral::vec4f(m_CameraMotion.GetPosition(), 0.0f);
		memset(m_SceneData.SH, 0, sizeof(m_SceneData.SH));
		insigne::copy_update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);
	}

	{
		m_IBLBakeProjection.near_plane = 0.01f;
		m_IBLBakeProjection.far_plane = 100.0f;
		m_IBLBakeProjection.fov = 90.0f;
		m_IBLBakeProjection.aspect_ratio = 1.0f;

		m_IBLBakeView.position = floral::vec3f(0.0f, 0.0f, 0.0f);
		m_IBLBakeView.look_at = floral::vec3f(1.0f, 0.0f, 0.0f);
		m_IBLBakeView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

		floral::mat4x4f p = floral::construct_perspective(m_IBLBakeProjection);
		floral::mat4x4f v = floral::construct_lookat_point(m_IBLBakeView);
		m_IBLBakeSceneData.WVP = p * v;
		m_IBLBakeSceneData.CameraPos = floral::vec4f(m_IBLBakeView.position, 0.0f);

		insigne::ubdesc_t desc;
		desc.region_size = 256;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		m_IBLBakeSceneUB = insigne::create_ub(desc);
		insigne::copy_update_ub(m_IBLBakeSceneUB, &m_IBLBakeSceneData, sizeof(IBLBakeSceneData), 0);
	}

	m_PrefilterConfigs.roughness = floral::vec2f(0.8f, 0.0f);
	{
		insigne::ubdesc_t desc;
		desc.region_size = 256;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;
		desc.alignment = 0;

		m_PrefilterUB = insigne::create_ub(desc);
		insigne::copy_update_ub(m_PrefilterUB, &m_PrefilterConfigs, sizeof(PrefilterConfigs), 0);
	}

	m_PreviewConfigs.texLod = floral::vec2f(0.0f, 0.0f);
	m_PreviewSpecConfigs.texLod = floral::vec2f(0.0f, 0.0f);
	{
		insigne::ubdesc_t desc;
		desc.region_size = 256;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;
		desc.alignment = 0;

		m_PreviewUB = insigne::create_ub(desc);
		m_PreviewPMREMUB = insigne::create_ub(desc);
		insigne::copy_update_ub(m_PreviewUB, &m_PreviewConfigs, sizeof(PreviewConfigs), 0);
		insigne::copy_update_ub(m_PreviewPMREMUB, &m_PreviewSpecConfigs, sizeof(PreviewConfigs), 0);
	}

	{
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		insigne::color_attachment_t atm;
		strcpy(atm.name, "main_color_cube");
		atm.texture_format = insigne::texture_format_e::hdr_rgb;
		atm.texture_dimension = insigne::texture_dimension_e::tex_cube;
		desc.color_attachments->push_back(atm);
		desc.width = k_faceSize; desc.height = k_faceSize;
		desc.has_depth = false;
		desc.color_has_mipmap = true;

		m_SpecularFB = insigne::create_framebuffer(desc);
	}

	{
		m_SHInputTexData = (f32*)g_SceneResourceAllocator.allocate(k_faceSize * k_faceSize * 24 * sizeof(f32));
		m_SHRadTexData = (f32*)g_SceneResourceAllocator.allocate(SIZE_KB(200));
		memset(m_SHRadTexData, 0, SIZE_KB(200));
		m_SHIrrTexData = (f32*)g_SceneResourceAllocator.allocate(SIZE_KB(200));
		memset(m_SHIrrTexData, 0, SIZE_KB(200));

		insigne::texture_desc_t desc;
		desc.width = k_previewFaceSize;
		desc.height = k_previewFaceSize;
		desc.format = insigne::texture_format_e::hdr_rgb_high;
		desc.min_filter = insigne::filtering_e::linear;
		desc.mag_filter = insigne::filtering_e::linear;
		desc.dimension = insigne::texture_dimension_e::tex_2d;
		desc.wrap_s = insigne::wrap_e::clamp_to_edge;
		desc.wrap_t = insigne::wrap_e::clamp_to_edge;
		desc.wrap_r = insigne::wrap_e::clamp_to_edge;
		desc.compression = insigne::texture_compression_e::no_compression;
		desc.has_mipmap = false;
		desc.data = nullptr;

		m_SHPreviewRadianceTex = insigne::create_texture(desc);
		m_SHPreviewIrradianceTex = insigne::create_texture(desc);

		desc.width = k_faceSize * 2;
		desc.height = k_faceSize * 2;
		m_PreviewTexture[size(ProjectionScheme::LightProbe)] = insigne::create_texture(desc);
		m_InputTexture[size(ProjectionScheme::LightProbe)] = insigne::create_texture(desc);

		desc.width = k_faceSize * 3;
		desc.height = k_faceSize * 2;
		m_PreviewTexture[size(ProjectionScheme::HStrip)] = insigne::create_texture(desc);

		desc.width = k_faceSize * 4;
		desc.height = k_faceSize * 2;
		m_PreviewTexture[size(ProjectionScheme::Equirectangular)] = insigne::create_texture(desc);
		m_InputTexture[size(ProjectionScheme::Equirectangular)] = insigne::create_texture(desc);

		desc.width = k_faceSize;
		desc.height = k_faceSize;
		desc.min_filter = insigne::filtering_e::linear_mipmap_linear;
		desc.dimension = insigne::texture_dimension_e::tex_cube;
		desc.has_mipmap = true;
		desc.data = nullptr;
		m_InputTexture[size(ProjectionScheme::HStrip)] = insigne::create_texture(desc);

		m_CurrentPreviewTexture = insigne::k_invalid_handle;
		m_Current3DPreviewTexture = insigne::k_invalid_handle;
	}

	m_TemporalArena->free_all();
	pfx_parser::PostEffectsDescription pfxDesc = pfx_parser::ParsePostFX(
			floral::path("tests/tech/sh_calculator/hdr_pfx.pfx"),
			m_TemporalArena);
	m_PostFXChain.Initialize(pfxDesc, floral::vec2f(commonCtx->window_width, commonCtx->window_height), m_PostFXArena);

	_LoadMaterial(&m_SHPreviewMSPair, floral::path("tests/tech/sh_calculator/probe_preview_sh.mat"));
	_LoadMaterial(&m_SHSkyPreviewMSPair, floral::path("tests/tech/sh_calculator/sky_preview_sh.mat"));
	_LoadMaterial(&m_PMREMBakeMSPair, floral::path("tests/tech/sh_calculator/pmrem_bake.mat"));
	_LoadMaterial(&m_PreviewMSPair[0], floral::path("tests/tech/sh_calculator/probe_preview_lightprobe.mat"));
	_LoadMaterial(&m_SkyPreviewMSPair[0], floral::path("tests/tech/sh_calculator/sky_preview_lightprobe.mat"));
	_LoadMaterial(&m_PreviewMSPair[1], floral::path("tests/tech/sh_calculator/probe_preview_hstrip.mat"));
	_LoadMaterial(&m_SkyPreviewMSPair[1], floral::path("tests/tech/sh_calculator/sky_preview_hstrip.mat"));
	_LoadMaterial(&m_PreviewMSPair[2], floral::path("tests/tech/sh_calculator/probe_preview_latlong.mat"));
	_LoadMaterial(&m_SkyPreviewMSPair[2], floral::path("tests/tech/sh_calculator/sky_preview_latlong.mat"));
	_LoadMaterial(&m_PMREMPreviewMSPair, floral::path("tests/tech/sh_calculator/probe_preview_hstrip.mat"));
	_LoadMaterial(&m_PMREMSkyPreviewMSPair, floral::path("tests/tech/sh_calculator/sky_preview_hstrip.mat"));

	insigne::helpers::assign_uniform_block(m_SHPreviewMSPair.material, "ub_Scene", 0, 0, m_UB);
	insigne::helpers::assign_uniform_block(m_SHSkyPreviewMSPair.material, "ub_Scene", 0, 0, m_UB);

	insigne::helpers::assign_uniform_block(m_PMREMBakeMSPair.material, "ub_Scene", 0, 0, m_IBLBakeSceneUB);
	insigne::helpers::assign_uniform_block(m_PMREMBakeMSPair.material, "ub_PrefilterConfigs", 0, 0, m_PrefilterUB);

	insigne::helpers::assign_uniform_block(m_PreviewMSPair[0].material, "ub_Scene", 0, 0, m_UB);
	insigne::helpers::assign_uniform_block(m_SkyPreviewMSPair[0].material, "ub_Scene", 0, 0, m_UB);
	insigne::helpers::assign_uniform_block(m_PreviewMSPair[1].material, "ub_Scene", 0, 0, m_UB);
	insigne::helpers::assign_uniform_block(m_SkyPreviewMSPair[1].material, "ub_Scene", 0, 0, m_UB);
	insigne::helpers::assign_uniform_block(m_PreviewMSPair[1].material, "ub_Preview", 0, 0, m_PreviewUB);
	insigne::helpers::assign_uniform_block(m_SkyPreviewMSPair[1].material, "ub_Preview", 0, 0, m_UB);
	insigne::helpers::assign_uniform_block(m_PreviewMSPair[2].material, "ub_Scene", 0, 0, m_UB);
	insigne::helpers::assign_uniform_block(m_SkyPreviewMSPair[2].material, "ub_Scene", 0, 0, m_UB);

	insigne::helpers::assign_uniform_block(m_PMREMPreviewMSPair.material, "ub_Scene", 0, 0, m_UB);
	insigne::helpers::assign_uniform_block(m_PMREMPreviewMSPair.material, "ub_Preview", 0, 0, m_PreviewPMREMUB);
	insigne::helpers::assign_texture(m_PMREMPreviewMSPair.material, "u_Tex", insigne::extract_color_attachment(m_SpecularFB, 0));
	insigne::helpers::assign_uniform_block(m_PMREMSkyPreviewMSPair.material, "ub_Scene", 0, 0, m_UB);
	insigne::helpers::assign_uniform_block(m_PMREMSkyPreviewMSPair.material, "ub_Preview", 0, 0, m_PreviewPMREMUB);
	insigne::helpers::assign_texture(m_PMREMSkyPreviewMSPair.material, "u_Tex", insigne::extract_color_attachment(m_SpecularFB, 0));

	m_CurrentPreviewMSPair = nullptr;
	m_CurrentSkyPreviewMSPair = nullptr;
}

//--------------------------------------------------------------------

void SHCalculator::_OnUpdate(const f32 i_deltaMs)
{
	// DebugUI
	ImGui::Begin("Controller");

	static s32 currentImage = 0;
	if (ImGui::Combo("Input image", &currentImage, k_Images, IM_ARRAYSIZE(k_Images)))
	{
		_LoadHDRImage(k_Images[currentImage]);
	}

	ImGui::Separator();

	if (m_ImgLoaded)
	{
		ImGui::Text("Image preview (LDR, color clamped to [0..1])");
		s32 width = -1;
		s32 height = -1;
		switch (m_CurrentProjectionScheme)
		{
		case ProjectionScheme::LightProbe:
			width = k_previewFaceSize * 2;
			height = k_previewFaceSize * 2;
			break;
		case ProjectionScheme::HStrip:
			width = k_previewFaceSize * 3;
			height = k_previewFaceSize * 2;
			break;
		case ProjectionScheme::Equirectangular:
			width = k_previewFaceSize * 4;
			height = k_previewFaceSize * 2;
			break;
		default:
			FLORAL_ASSERT(false);
			break;
		}
		ImGui::Image(&m_CurrentPreviewTexture, ImVec2(width, height));
		ImGui::Text("Projection: %s", k_ProjectionSchemeStr[size(m_CurrentProjectionScheme)]);
		if (!m_ImgToneMapped)
		{
			ImGui::Text("HDR range: [%4.2f, %4.2f, %4.2f] - [%4.2f, %4.2f, %4.2f]",
					m_MinHDR.x, m_MinHDR.y, m_MinHDR.z,
					m_MaxHDR.x, m_MaxHDR.y, m_MaxHDR.z);
		}
		else
		{
			ImGui::Text("HDR range (tone-mapped): [%4.2f, %4.2f, %4.2f] - [%4.2f, %4.2f, %4.2f]",
					m_MinHDR.x, m_MinHDR.y, m_MinHDR.z,
					m_MaxHDR.x, m_MaxHDR.y, m_MaxHDR.z);
		}
		if (ImGui::Button("Preview 3D##input"))
		{
			m_CurrentPreviewMSPair = &m_PreviewMSPair[size(m_CurrentProjectionScheme)];
			m_CurrentSkyPreviewMSPair = &m_SkyPreviewMSPair[size(m_CurrentProjectionScheme)];
			insigne::helpers::assign_texture(m_CurrentPreviewMSPair->material, "u_Tex", m_Current3DPreviewTexture);
			insigne::helpers::assign_texture(m_CurrentSkyPreviewMSPair->material, "u_Tex", m_Current3DPreviewTexture);
			if (m_CurrentProjectionScheme == ProjectionScheme::HStrip)
			{
				insigne::helpers::assign_uniform_block(m_CurrentPreviewMSPair->material, "ub_Preview", 0, 0, m_PreviewUB);
				insigne::helpers::assign_uniform_block(m_CurrentSkyPreviewMSPair->material, "ub_Preview", 0, 0, m_PreviewUB);
			}
		}
		if (m_CurrentProjectionScheme == ProjectionScheme::HStrip)
		{
			ImGui::SameLine();
			if (ImGui::SliderFloat("LOD##input", &m_PreviewConfigs.texLod.x, 0.0f, 9.0f))
			{
				insigne::copy_update_ub(m_PreviewUB, &m_PreviewConfigs, sizeof(PreviewConfigs), 0);
			}
		}
	}

	if (ImGui::Button("Calculate"))
	{
		_ComputeSH();
		if (m_CurrentProjectionScheme == ProjectionScheme::HStrip)
		{
			insigne::helpers::assign_texture(m_PMREMBakeMSPair.material, "u_Tex", m_InputTexture[size(m_CurrentProjectionScheme)]);
			m_NeedBakePMREM = true;
		}
	}

	if (m_PMREMReady && m_SHReady)
	{
		ImGui::Separator();
		if (ImGui::Button("Save to file"))
		{
			insigne::texture_desc_t texDesc;
			texDesc.width = k_faceSize;
			texDesc.height = k_faceSize;
			texDesc.format = insigne::texture_format_e::hdr_rgb_high;
			texDesc.dimension = insigne::texture_dimension_e::tex_cube;
			texDesc.wrap_s = insigne::wrap_e::clamp_to_edge;
			texDesc.wrap_t = insigne::wrap_e::clamp_to_edge;
			texDesc.wrap_r = insigne::wrap_e::clamp_to_edge;
			texDesc.compression = insigne::texture_compression_e::no_compression;
			texDesc.has_mipmap = true;
			const size dataSize = insigne::calculate_texture_memsize(texDesc);

			m_PMREMImageData = (f32*)m_TemporalArena->allocate(dataSize);
			m_PMREMPromisedFrame = insigne::schedule_framebuffer_capture(m_SpecularFB, m_PMREMImageData);
			m_IsCapturingPMREMData = true;
		}
		if (m_TexFileWritten)
		{
			ImGui::SameLine();
			ImGui::Text("Saved!");
		}
	}

	if (m_IsCapturingPMREMData && insigne::get_current_frame_idx() >= m_PMREMPromisedFrame)
	{
		floral::file_info oSHFile = floral::open_output_file("out.cbsh");
		floral::output_file_stream oSHStream;
		floral::map_output_file(oSHFile, &oSHStream);
		for (s32 i = 0; i < 9; i++)
		{
			const floral::vec3f& coeff = m_SHComputeTaskData.OutputCoeffs[i];
			oSHStream.write(coeff);
		}
		floral::close_file(oSHFile);

		f32* pData = m_PMREMImageData;
		floral::file_info oFile = floral::open_output_file("out.cbtex");
		floral::output_file_stream oStream;
		floral::map_output_file(oFile, &oStream);

		tex_loader::TextureHeader header;
		header.textureType = tex_loader::Type::PMREM;
		header.colorRange = tex_loader::ColorRange::HDR;
		header.colorSpace = tex_loader::ColorSpace::Linear;
		header.colorChannel = tex_loader::ColorChannel::RGB;
		header.encodedGamma = 0.5f;
		header.mipsCount = (s32)log2(k_faceSize) + 1;
		header.resolution = k_faceSize;
		header.compression = tex_loader::Compression::DXT;

		oStream.write(header);
		for (s32 i = 0; i < 6; i++)
		{
			for (s32 j = 0; j < header.mipsCount; j++)
			{
				s32 mipSize = k_faceSize >> j;
				p8 rgbaData = m_TemporalArena->allocate_array<u8>(mipSize * mipSize * 4);
				convert_to_rgbm(pData, rgbaData, mipSize, mipSize, 0.5f);
				size compressedSize = 0;
				p8 compressedData = compress_dxt(rgbaData, mipSize, mipSize, 4, &compressedSize, m_TemporalArena);
				oStream.write_bytes(compressedData, compressedSize);
				m_TemporalArena->free(compressedData);
				m_TemporalArena->free(rgbaData);

				pData += (mipSize * mipSize * 3);
			}
		}
		floral::close_file(oFile);

		m_TemporalArena->free(m_PMREMImageData);
		m_PMREMImageData = nullptr;
		m_IsCapturingPMREMData = false;
		m_TexFileWritten = true;
	}

	ImGui::Separator();
	if (m_PMREMReady)
	{
		ImGui::Text("PMREM");
		if (ImGui::Button("Preview 3D##pmrem"))
		{
			m_CurrentPreviewMSPair = &m_PMREMPreviewMSPair;
			m_CurrentSkyPreviewMSPair = &m_PMREMSkyPreviewMSPair;
		}
		ImGui::SameLine();
		if (ImGui::SliderFloat("LOD##pmrem", &m_PreviewSpecConfigs.texLod.x, 0.0f, 9.0f))
		{
			insigne::copy_update_ub(m_PreviewPMREMUB, &m_PreviewSpecConfigs, sizeof(PreviewConfigs), 0);
		}
	}
	else
	{
		ImGui::Text("Please calculate the PMREM");
	}

	ImGui::Separator();
	if (m_SHReady)
	{
		ImGui::Text("Results from SH Coeffs:");
		ImGui::SameLine();
		if (ImGui::Button("Preview 3D##sh"))
		{
			m_CurrentPreviewMSPair = &m_SHPreviewMSPair;
			m_CurrentSkyPreviewMSPair = &m_SHSkyPreviewMSPair;
		}
		ImGui::Text("Radiance");
		ImGui::SameLine(150, 20);
		ImGui::Text("Irradiance");
		ImGui::Image(&m_SHPreviewRadianceTex, ImVec2(k_previewFaceSize, k_previewFaceSize));
		ImGui::SameLine(150, 20);
		ImGui::Image(&m_SHPreviewIrradianceTex, ImVec2(k_previewFaceSize, k_previewFaceSize));
		if (ImGui::CollapsingHeader("SH coeffs"))
		{
			for (u32 i = 0; i < 9; i++)
			{
				c8 bandStr[128];
				memset(bandStr, 0, 128);
				sprintf(bandStr, "#%d", i + 1);
				ImGui::InputFloat3(bandStr, &m_SHComputeTaskData.OutputCoeffs[i].x, 5, ImGuiInputTextFlags_ReadOnly);
			}
		}
	}
	else
	{
		ImGui::Text("Please calculate the SH");
	}
	ImGui::End();

	// Logic update
	m_CameraMotion.OnUpdate(i_deltaMs);
	m_SceneData.viewProjectionMatrix = m_CameraMotion.GetWVP();
	m_SceneData.cameraPosition = floral::vec4f(m_CameraMotion.GetPosition(), 0.0f);
	insigne::copy_update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

	if (m_ComputingSH)
	{
		if (refrain2::CheckForCounter(m_Counter, 0))
		{
			// upload!!!
			insigne::texture_desc_t desc;
			desc.width = k_previewFaceSize;
			desc.height = k_previewFaceSize;
			desc.format = insigne::texture_format_e::hdr_rgb_high;
			desc.min_filter = insigne::filtering_e::linear;
			desc.mag_filter = insigne::filtering_e::linear;
			desc.dimension = insigne::texture_dimension_e::tex_2d;
			desc.wrap_s = insigne::wrap_e::clamp_to_edge;
			desc.wrap_t = insigne::wrap_e::clamp_to_edge;
			desc.compression = insigne::texture_compression_e::no_compression;
			desc.has_mipmap = false;

			const size dataSize = insigne::prepare_texture_desc(desc);
			insigne::copy_update_texture(m_SHPreviewIrradianceTex, m_SHIrrTexData, dataSize);
			insigne::copy_update_texture(m_SHPreviewRadianceTex, m_SHRadTexData, dataSize);

			for (u32 i = 0; i < 9; i++)
			{
				m_SceneData.SH[i] = floral::vec4f(m_SHComputeTaskData.OutputCoeffs[i], 0.0f);
			}
			insigne::copy_update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

			m_ComputingSH = false;
			m_SHReady = true;
		}
	}

	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(2.0f, 0.0f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 2.0f, 0.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 0.0f, 2.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
}

//--------------------------------------------------------------------

void SHCalculator::_OnRender(const f32 i_deltaMs)
{
	if (m_NeedBakePMREM)
	{
		s32 mipsCount = (s32)log2(k_faceSize) + 1;
		for (s32 m = 0; m < mipsCount; m++)
		{
			f32 r = (f32)m / 4.0f;
			m_PrefilterConfigs.roughness = floral::vec2f(r, 0.0f);
			insigne::copy_update_ub(m_PrefilterUB, &m_PrefilterConfigs, sizeof(PrefilterConfigs), 0);
			for (size i = 0; i < 6; i++)
			{
				m_IBLBakeView.look_at = s_lookAts[i];
				m_IBLBakeView.up_direction = s_upDirs[i];
				floral::mat4x4f p = floral::construct_perspective(m_IBLBakeProjection);
				floral::mat4x4f v = floral::construct_lookat_point(m_IBLBakeView);
				m_IBLBakeSceneData.WVP = p * v;
				insigne::copy_update_ub(m_IBLBakeSceneUB, &m_IBLBakeSceneData, sizeof(IBLBakeSceneData), 0);

				insigne::begin_render_pass(m_SpecularFB, s_faces[i], m);
				insigne::draw_surface<geo3d::SurfaceP>(m_PMREMSkybox.vb, m_PMREMSkybox.ib, m_PMREMBakeMSPair.material);
				insigne::end_render_pass(m_SpecularFB);
				insigne::dispatch_render_pass();
			}
		}
		m_NeedBakePMREM = false;
		m_PMREMReady = true;
	}

	m_PostFXChain.BeginMainOutput();
	if (m_CurrentPreviewMSPair != nullptr)
	{
		insigne::draw_surface<geo3d::SurfaceP>(m_Sphere.vb, m_Sphere.ib, m_CurrentPreviewMSPair->material);
	}
	if (m_CurrentSkyPreviewMSPair != nullptr)
	{
		insigne::draw_surface<geo3d::SurfaceP>(m_Skysphere.vb, m_Skysphere.ib, m_CurrentSkyPreviewMSPair->material);
	}
	debugdraw::Render(m_SceneData.viewProjectionMatrix);
	m_PostFXChain.EndMainOutput();

	m_PostFXChain.Process();

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	m_PostFXChain.Present();
	RenderImGui();
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();

	insigne::dispatch_render_pass();
}

//--------------------------------------------------------------------

void SHCalculator::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);

	g_StreammingAllocator.free(m_PostFXArena);
	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_TemporalArena);
	m_TemporalArena = nullptr;

	g_SceneResourceAllocator.free_all();

	insigne::unregister_surface_type<geo2d::SurfacePT>();
	insigne::unregister_surface_type<geo3d::SurfaceP>();
}

//--------------------------------------------------------------------

void SHCalculator::_ComputeSH()
{
	if (m_ComputingSH || !m_ImgLoaded)
	{
		return;
	}

	m_ComputingSH = true;
	m_SHReady = false;

	m_TemporalArena->free_all();
	m_SHComputeTaskData.InputTexture = m_SHInputTexData;
	m_SHComputeTaskData.OutputRadianceTex = m_SHRadTexData;
	m_SHComputeTaskData.OutputIrradianceTex = m_SHIrrTexData;
	m_SHComputeTaskData.Resolution = k_faceSize;
	m_SHComputeTaskData.Projection = s32(m_CurrentProjectionScheme);
	m_SHComputeTaskData.LocalMemoryArena = m_TemporalArena->allocate_arena<LinearArena>(SIZE_MB(4));
	m_SHComputeTaskData.DebugFaceIndex = 0;

	m_Counter.store(1);
	refrain2::Task newTask;
	newTask.pm_Instruction = &SHCalculator::ComputeSHCoeffs;
	newTask.pm_Data = (voidptr)&m_SHComputeTaskData;
	newTask.pm_Counter = &m_Counter;
	refrain2::g_TaskManager->PushTask(newTask);
}

//--------------------------------------------------------------------

void SHCalculator::_LoadHDRImage(const_cstr i_fileName)
{
	// clear 3d preview
	m_CurrentPreviewMSPair = nullptr;
	m_CurrentSkyPreviewMSPair = nullptr;
	m_PMREMReady = false;
	m_SHReady = false;
	m_ImgToneMapped = false;
	m_TexFileWritten = false;
	m_ComputingSH = false;
	m_ImgLoaded = false;

	m_TemporalArena->free_all();

	if (strcmp(i_fileName, "<none>") == 0)
	{
		return;
	}

	c8 imagePath[1024];
	sprintf(imagePath, "tests/tech/sh_calculator/imgs/%s", i_fileName);
	CLOVER_DEBUG("Loading image: %s", imagePath);
	s32 x, y, n;
	f32* imageData = stbi_loadf(imagePath, &x, &y, &n, 0);

	f32 maxRange[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	s32 pixelsCount = x * y;
	for (s32 i = 0; i < pixelsCount; i++)
	{
		for (s32 c = 0; c < n; c++)
		{
			FLORAL_ASSERT(imageData[i * n + c] >= 0.0f);
			if (imageData[i * n + c] > maxRange[c])
			{
				maxRange[c] = imageData[i * n + c];
			}
		}
	}

	if (maxRange[0] > 36.0f || maxRange[1] > 36.0f || maxRange[2] > 36.0f)
	{
		CLOVER_WARNING("Tonemapping the hdr input because the hdr range is outside 36.0f");
		for (s32 i = 0; i < pixelsCount; i++)
		{
			floral::vec3f toneMapHDRColor = hdr_tonemap(&imageData[i * 3]);
			imageData[i * 3] = toneMapHDRColor.x;
			imageData[i * 3 + 1] = toneMapHDRColor.y;
			imageData[i * 3 + 2] = toneMapHDRColor.z;
		}
		m_ImgToneMapped = true;
	}

	m_CurrentProjectionScheme = get_projection_from_size(x, y);
	FLORAL_ASSERT_MSG(m_CurrentProjectionScheme != ProjectionScheme::Invalid, "Invalid projection scheme, check image's resolution!");
	FLORAL_ASSERT_MSG(n == 3, "Invalid channels count, only supports RGB HDR images");

	m_MinHDR = floral::vec3f(9999.0f, 9999.0f, 9999.0f);
	m_MaxHDR = floral::vec3f(0.0f, 0.0f, 0.0f);
	for (s32 i = 0; i < x * y; i++)
	{
		// red
		if (imageData[i * 3] < m_MinHDR.x) m_MinHDR.x = imageData[i * 3];
		if (imageData[i * 3] > m_MaxHDR.x) m_MaxHDR.x = imageData[i * 3];

		// green
		if (imageData[i * 3 + 1] < m_MinHDR.y) m_MinHDR.y = imageData[i * 3 + 1];
		if (imageData[i * 3 + 1] > m_MaxHDR.y) m_MaxHDR.y = imageData[i * 3 + 1];

		// blue
		if (imageData[i * 3 + 2] < m_MinHDR.z) m_MinHDR.z = imageData[i * 3 + 2];
		if (imageData[i * 3 + 2] > m_MaxHDR.z) m_MaxHDR.z = imageData[i * 3 + 2];
	}

	// prepare preview texture and input texture
	{
		insigne::texture_handle_t previewTexture = m_PreviewTexture[size(m_CurrentProjectionScheme)];
		insigne::texture_handle_t inputTexture = m_InputTexture[size(m_CurrentProjectionScheme)];
		// in order to fully display loaded image, we have to transfer all the image data
		switch (m_CurrentProjectionScheme)
		{
		case ProjectionScheme::LightProbe:
		case ProjectionScheme::Equirectangular:
			insigne::copy_update_texture(previewTexture, imageData, x * y * 3 * sizeof(f32));
			insigne::copy_update_texture(inputTexture, imageData, x * y * 3 * sizeof(f32));
			break;
		case ProjectionScheme::HStrip:
		{
			// re-arrange hstrip to display preview more effectively
			{
				insigne::texture_desc_t desc;
				desc.format = insigne::texture_format_e::hdr_rgb_high;
				desc.min_filter = insigne::filtering_e::linear;
				desc.mag_filter = insigne::filtering_e::linear;
				desc.dimension = insigne::texture_dimension_e::tex_2d;
				desc.wrap_s = insigne::wrap_e::clamp_to_edge;
				desc.wrap_t = insigne::wrap_e::clamp_to_edge;
				desc.compression = insigne::texture_compression_e::no_compression;
				desc.has_mipmap = false;
				desc.width = k_faceSize * 3;
				desc.height = k_faceSize * 2;

				insigne::prepare_texture_desc(desc);
				f32* previewTexData = (f32*)desc.data;
				trim_image(imageData, previewTexData, 0, 0, k_faceSize * 3, k_faceSize,
						k_faceSize * 6, k_faceSize, 3);
				previewTexData += (desc.width * desc.height * 3) / 2;
				trim_image(imageData, previewTexData, k_faceSize * 3, 0, k_faceSize * 3, k_faceSize,
						k_faceSize * 6, k_faceSize, 3);
				insigne::update_texture(previewTexture, desc);
			}

			// hstrip input texture is a texcube instead of a tex2d and we have to build mipmaps
			{
				insigne::texture_desc_t desc;
				desc.width = k_faceSize;
				desc.height = k_faceSize;
				desc.format = insigne::texture_format_e::hdr_rgb_high;
				desc.min_filter = insigne::filtering_e::linear_mipmap_linear;
				desc.mag_filter = insigne::filtering_e::linear;
				desc.dimension = insigne::texture_dimension_e::tex_cube;
				desc.wrap_s = insigne::wrap_e::clamp_to_edge;
				desc.wrap_t = insigne::wrap_e::clamp_to_edge;
				desc.wrap_r = insigne::wrap_e::clamp_to_edge;
				desc.compression = insigne::texture_compression_e::no_compression;
				desc.has_mipmap = true;

				insigne::prepare_texture_desc(desc);
				f32* imgData3D = (f32*)desc.data;
				f32* pOutBuffer = imgData3D;
				s32 mipsCount = (s32)log2(k_faceSize) + 1;
				for (s32 i = 0; i < 6; i++)
				{
					// mip 0
					CLOVER_DEBUG("Building face %d mip 0 for h-strip...", i);
					f32* mip0Data = pOutBuffer;
					trim_image(imageData, pOutBuffer, i * k_faceSize, 0, k_faceSize, k_faceSize,
							k_faceSize * 6, k_faceSize, 3);
					pOutBuffer += (k_faceSize * k_faceSize * 3);

					// rest of the mips chain
					s32 texSize = k_faceSize;
					for (s32 m = 0; m < mipsCount; m++)
					{
						CLOVER_DEBUG("Building face %d mip %d for h-strip...", i, m + 1);
						texSize >>= 1;
						stbir_resize_float(mip0Data, k_faceSize, k_faceSize, 0, pOutBuffer, texSize, texSize, 0, 3);
						pOutBuffer += (texSize * texSize * 3);
					}
				}
				insigne::update_texture(inputTexture, desc);
			}
			break;
		}
		default:
			FLORAL_ASSERT(false);
			break;
		}

		insigne::dispatch_render_pass();
		m_CurrentPreviewTexture = previewTexture;
		m_Current3DPreviewTexture = inputTexture;
	}

	memcpy(m_SHInputTexData, imageData, x * y * 3 * sizeof(f32));
	stbi_image_free(imageData);
	m_ImgLoaded = true;
}

// -------------------------------------------------------------------

void SHCalculator::_LoadMaterial(mat_loader::MaterialShaderPair* o_msPair, const floral::path& i_path)
{
	m_TemporalArena->free_all();
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(i_path, m_TemporalArena);
	bool matLoadResult = mat_loader::CreateMaterial(o_msPair, matDesc, m_TemporalArena, m_MaterialDataArena);
	FLORAL_ASSERT(matLoadResult);
}

// -------------------------------------------------------------------

refrain2::Task SHCalculator::ComputeSHCoeffs(voidptr i_data)
{
	SHComputeData* input = (SHComputeData*)i_data;
	s32 sqrtNSamples = 100;
	s32 NSamples = sqrtNSamples * sqrtNSamples;
	sh_sample* samples = (sh_sample*)input->LocalMemoryArena->allocate_array<sh_sample>(NSamples);
	sh_setup_spherical_samples(samples, sqrtNSamples);

	highp_vec3_t shResult[9];
	sh_project_light_image(input->InputTexture, input->Projection, input->Resolution, NSamples, 9, samples, shResult);

	for (s32 i = 0; i < 9; i++)
	{
		CLOVER_DEBUG("(%f; %f; %f)", shResult[i].x, shResult[i].y, shResult[i].z);

		input->OutputCoeffs[i].x = (f32)shResult[i].x;
		input->OutputCoeffs[i].y = (f32)shResult[i].y;
		input->OutputCoeffs[i].z = (f32)shResult[i].z;
	}

	reconstruct_sh_radiance_light_probe(shResult, input->OutputRadianceTex, k_previewFaceSize, 1024);
	reconstruct_sh_irradiance_light_probe(shResult, input->OutputIrradianceTex, k_previewFaceSize, 1024);

	CLOVER_VERBOSE("Compute finished");
	return refrain2::Task();
}

//--------------------------------------------------------------------
}
}
