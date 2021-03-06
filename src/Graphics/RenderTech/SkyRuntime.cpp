#include "SkyRuntime.h"

#include <calyx/context.h>

#include <floral/comgeo/shapegen.h>
#include <floral/containers/array.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/MaterialParser.h"
#include "Graphics/CbModelLoader.h"
#include "Graphics/DebugDrawer.h"

namespace stone
{
namespace tech
{
//-------------------------------------------------------------------

static const floral::vec3f k_upDirection = floral::vec3f(0.0f, 0.0f, 1.0f);

SkyRuntime::SkyRuntime()
	: m_TexDataArenaRegion { "stone/dynamic/sky runtime", SIZE_MB(128), &m_TexDataArena }
{
}

//-------------------------------------------------------------------

SkyRuntime::~SkyRuntime()
{
}

//-------------------------------------------------------------------

ICameraMotion* SkyRuntime::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr SkyRuntime::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void SkyRuntime::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	floral::relative_path wdir = floral::build_relative_path("tests/tech/sky");
	floral::push_directory(m_FileSystem, wdir);

	g_MemoryManager.initialize_allocator(m_TexDataArenaRegion);
	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(16));
	m_PostFXArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));
	m_ModelDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(1));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(256));

	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
	m_AspectRatio = (f32)commonCtx->window_width / (f32)commonCtx->window_height;

	m_MemoryArena->free_all();
	floral::relative_path pfxPath = floral::build_relative_path("postfx.pfx");
	pfx_parser::PostEffectsDescription pfxDesc = pfx_parser::ParsePostFX(m_FileSystem, pfxPath, m_MemoryArena);
	m_PostFXChain.Initialize(m_FileSystem, pfxDesc, floral::vec2f(commonCtx->window_width, commonCtx->window_height), m_PostFXArena);

	// register surfaces
	insigne::register_surface_type<geo2d::SurfacePT>();
	insigne::register_surface_type<geo3d::SurfacePNTT>();
	insigne::register_surface_type<geo3d::SurfacePNC>();

	floral::inplace_array<geo2d::VertexPT, 4> vertices;
	vertices.push_back({ { -1.0f, 1.0f }, { 0.0f, 1.0f } });
	vertices.push_back({ { -1.0f, -1.0f }, { 0.0f, 0.0f } });
	vertices.push_back({ { 1.0f, -1.0f }, { 1.0f, 0.0f } });
	vertices.push_back({ { 1.0f, 1.0f }, { 1.0f, 1.0f } });

	floral::inplace_array<s32, 6> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(0);

	m_Quad = helpers::CreateSurfaceGPU(&vertices[0], 4, sizeof(geo2d::VertexPT),
			&indices[0], 6, insigne::buffer_usage_e::static_draw, true);

	// prebake splitsum
	{
		m_MemoryArena->free_all();
		mat_loader::MaterialShaderPair splitSumMSPair;
		floral::relative_path matPath = floral::build_relative_path("pbr_splitsum.mat");
		mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
		const bool ssMaterialResult = mat_loader::CreateMaterial(&splitSumMSPair, m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
		FLORAL_ASSERT(ssMaterialResult);

		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		insigne::color_attachment_t atm;
		strcpy(atm.name, "main_color");
		atm.texture_format = insigne::texture_format_e::hdr_rg;
		atm.texture_dimension = insigne::texture_dimension_e::tex_2d;
		desc.color_attachments->push_back(atm);
		desc.width = 512; desc.height = 512;
		desc.has_depth = false;

		insigne::framebuffer_handle_t fb = insigne::create_framebuffer(desc);
		insigne::dispatch_render_pass();

		insigne::begin_render_pass(fb);
		insigne::draw_surface<geo2d::SurfacePT>(m_Quad.vb, m_Quad.ib, splitSumMSPair.material);
		insigne::end_render_pass(fb);
		insigne::dispatch_render_pass();

		m_SplitSumTexture = insigne::extract_color_attachment(fb, 0);
	}

	{
		m_MemoryArena->free_all();
		cbmodel::Model<geo3d::VertexPNTT> model = cbmodel::LoadModelData<geo3d::VertexPNTT>(m_FileSystem,
				floral::build_relative_path("DamagedHelmet.gltf_mesh_0.cbmodel"),
				cbmodel::VertexAttribute::Position | cbmodel::VertexAttribute::Normal | cbmodel::VertexAttribute::Tangent | cbmodel::VertexAttribute::TexCoord,
				m_MemoryArena, m_ModelDataArena);

		m_HelmetSurfaceGPU = helpers::CreateSurfaceGPU(model.verticesData, model.verticesCount, sizeof(geo3d::VertexPNTT),
				model.indicesData, model.indicesCount, insigne::buffer_usage_e::static_draw, false);

		m_MemoryArena->free_all();
		mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem,
				floral::build_relative_path("pbr_helmet.mat"), m_MemoryArena);
		const bool pbrMaterialResult = mat_loader::CreateMaterial(&m_HelmetMSPair, m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
		FLORAL_ASSERT(pbrMaterialResult == true);
		insigne::helpers::assign_texture(m_HelmetMSPair.material, "u_SplitSumTex", m_SplitSumTexture);
	}

	floral::fast_fixed_array<geo3d::VertexPNC, FreelistArena> sphereVertices;
	floral::fast_fixed_array<s32, FreelistArena> sphereIndices;

	m_MemoryArena->free_all();
	const size numIndices = 1 << 18;
	sphereVertices.reserve(numIndices, m_MemoryArena);
	sphereIndices.reserve(numIndices, m_MemoryArena);
	sphereVertices.resize(8192);
	sphereIndices.resize(8192);
	floral::reset_generation_transforms_stack();
	floral::geo_generate_result_t genResult = floral::generate_unit_icosphere_3d(
			3, 0, sizeof(geo3d::VertexPNC),
			floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal,
			&sphereVertices[0], &sphereIndices[0]);
	sphereVertices.resize(genResult.vertices_generated);
	sphereIndices.resize(genResult.indices_generated);
	m_Surface = helpers::CreateSurfaceGPU(&sphereVertices[0], genResult.vertices_generated, sizeof(geo3d::VertexPNC),
			&sphereIndices[0], genResult.indices_generated, insigne::buffer_usage_e::static_draw, true);

	BakedDataInfos bakedDataInfos;
	{
		m_MemoryArena->free_all();
		floral::relative_path iFilePath = floral::build_relative_path("sky.meta");
		floral::file_info iFile = floral::open_file_read(m_FileSystem, iFilePath);
		FLORAL_ASSERT(iFile.file_size > 0);
		floral::file_stream iStream;
		iStream.buffer = (p8)m_MemoryArena->allocate(iFile.file_size);
		floral::read_all_file(iFile, iStream);
		floral::close_file(iFile);
		iStream.read(&bakedDataInfos);
		iStream.read(&m_SkyFixedConfigs);
	}

	m_MemoryArena->free_all();
	floral::relative_path matPath = floral::build_relative_path("sky.mat");
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	bool createResult = mat_loader::CreateMaterial(&m_MSPair, m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);

	matPath = floral::build_relative_path("sphere.mat");
	matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	createResult = mat_loader::CreateMaterial(&m_SphereMSPair, m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);

	matPath = floral::build_relative_path("convo.mat");
	matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	createResult = mat_loader::CreateMaterial(&m_ConvoMSPair, m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);

	m_TransmittanceTexture = LoadRawHDRTexture2D("transmittance_texture.rtex2d",
			bakedDataInfos.transmittanceTextureWidth, bakedDataInfos.transmittanceTextureHeight, 3);
	m_ScatteringTexture = LoadRawHDRTexture3D("scattering_texture.rtex3d",
			bakedDataInfos.scatteringTextureWidth, bakedDataInfos.scatteringTextureHeight, bakedDataInfos.scatteringTextureDepth, 4);
	m_IrradianceTexture = LoadRawHDRTexture2D("irradiance_texture.rtex2d",
			bakedDataInfos.irrandianceTextureWidth, bakedDataInfos.irrandianceTextureHeight, 3);

	insigne::helpers::assign_texture(m_MSPair.material, "u_TransmittanceTex", m_TransmittanceTexture);
	insigne::helpers::assign_texture(m_SphereMSPair.material, "u_TransmittanceTex", m_TransmittanceTexture);
	insigne::helpers::assign_texture(m_HelmetMSPair.material, "u_TransmittanceTex", m_TransmittanceTexture);
	insigne::helpers::assign_texture(m_MSPair.material, "u_ScatteringTex", m_ScatteringTexture);
	insigne::helpers::assign_texture(m_MSPair.material, "u_IrradianceTex", m_IrradianceTexture);
	insigne::helpers::assign_texture(m_SphereMSPair.material, "u_IrradianceTex", m_IrradianceTexture);
	insigne::helpers::assign_texture(m_HelmetMSPair.material, "u_IrradianceTex", m_IrradianceTexture);

	m_FovY = 50.0f;
	m_ProjectionMatrix = floral::construct_infinity_perspective(0.01f, m_FovY, m_AspectRatio);
	/*
	const f32 k_orthoSize = 20.0f;
	const f32 k_orthoWidth = k_orthoSize * m_AspectRatio;
	m_ProjectionMatrix = floral::construct_orthographic_lh(-k_orthoWidth * 0.5f, k_orthoWidth * 0.5f,
			k_orthoSize * 0.5f, -k_orthoSize * 0.5f, 0.0f, 100.0f);
	*/
	m_SceneData.viewFromClip = m_ProjectionMatrix.get_inverse();
	m_CamPos = floral::vec3f(-1.0f, -3.0f, 1.0f);
	m_LookAt = floral::vec3f(0.0f, 0.0f, 0.0f);
	floral::mat4x4f v = floral::construct_lookat_point(
			k_upDirection,
			m_CamPos, m_LookAt);
	m_ViewProjectionMatrix = m_ProjectionMatrix * v;
	m_SceneData.modelFromView = v.get_inverse();

	{
		insigne::ubdesc_t desc;
		desc.region_size = floral::next_pow2(size(sizeof(SceneData)));
		desc.data = &m_SceneData;
		desc.data_size = sizeof(SceneData);
		desc.usage = insigne::buffer_usage_e::dynamic_draw;
		m_SceneUB = insigne::copy_create_ub(desc);
		insigne::helpers::assign_uniform_block(m_MSPair.material, "ub_Scene", 0, 0, m_SceneUB);
	}

	m_ObjectSceneData.cameraPosition = floral::vec4f(m_CamPos, 1.0f);
	m_ObjectSceneData.transformMatrix = floral::construct_Xrotation3d(-floral::pi * 0.5f);
	m_ObjectSceneData.viewProjectionMatrix = m_ViewProjectionMatrix;
	{
		insigne::ubdesc_t desc;
		desc.region_size = floral::next_pow2(size(sizeof(ObjectSceneData)));
		desc.data = &m_ObjectSceneData;
		desc.data_size = sizeof(ObjectSceneData);
		desc.usage = insigne::buffer_usage_e::dynamic_draw;
		m_ObjectSceneUB = insigne::copy_create_ub(desc);
		insigne::helpers::assign_uniform_block(m_SphereMSPair.material, "ub_Scene", 0, 0, m_ObjectSceneUB);
		insigne::helpers::assign_uniform_block(m_HelmetMSPair.material, "ub_Scene", 0, 0, m_ObjectSceneUB);
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = floral::next_pow2(size(sizeof(ConvolutionSceneData)));
		desc.data = nullptr;
		desc.data_size = sizeof(ConvolutionSceneData);
		desc.usage = insigne::buffer_usage_e::dynamic_draw;
		m_ConvolutionUB = insigne::create_ub(desc);
		insigne::helpers::assign_uniform_block(m_ConvoMSPair.material, "ub_Scene", 0, 0, m_ConvolutionUB);
	}

	m_SunZenith = 1.564f;
	m_SunAzimuth = 0.0f;
	m_ConfigsData.camera = floral::vec4f(m_CamPos, 1.0f);
	m_ConfigsData.whitePoint = floral::vec4f(1.0f);
	m_ConfigsData.earthCenter = floral::vec4f(0.0f, 0.0f, -m_SkyFixedConfigs.bottomRadius, 1.0f);
	m_ConfigsData.sunDirection = floral::vec4f(
			cosf(m_SunAzimuth) * sinf(m_SunZenith),
			sinf(m_SunAzimuth) * sinf(m_SunZenith),
			cosf(m_SunZenith), 0.0f);
	m_ConfigsData.sunSize = floral::vec2f(tanf(m_SkyFixedConfigs.sunAngularRadius), cosf(m_SkyFixedConfigs.sunAngularRadius));
	m_ConfigsData.exposure = 10.0f;
	m_ConfigsData.unitLengthInMeters = m_SkyFixedConfigs.unitLengthInMeters;
	{
		insigne::ubdesc_t desc;
		desc.region_size = floral::next_pow2(size(sizeof(ConfigsData)));
		desc.data = &m_ConfigsData;
		desc.data_size = sizeof(ConfigsData);
		desc.usage = insigne::buffer_usage_e::dynamic_draw;
		m_ConfigsUB = insigne::copy_create_ub(desc);
		insigne::helpers::assign_uniform_block(m_MSPair.material, "ub_Configs", 0, 0, m_ConfigsUB);
		insigne::helpers::assign_uniform_block(m_SphereMSPair.material, "ub_Configs", 0, 0, m_ConfigsUB);
		insigne::helpers::assign_uniform_block(m_HelmetMSPair.material, "ub_Configs", 0, 0, m_ConfigsUB);
	}

	m_TextureInfoData.transmittanceTextureWidth = bakedDataInfos.transmittanceTextureWidth;
	m_TextureInfoData.transmittanceTextureHeight = bakedDataInfos.transmittanceTextureHeight;
	m_TextureInfoData.scatteringTextureRSize = bakedDataInfos.scatteringTextureRSize;
	m_TextureInfoData.scatteringTextureMuSize = bakedDataInfos.scatteringTextureMuSize;
	m_TextureInfoData.scatteringTextureMuSSize = bakedDataInfos.scatteringTextureMuSSize;
	m_TextureInfoData.scatteringTextureNuSize = bakedDataInfos.scatteringTextureNuSize;
	m_TextureInfoData.scatteringTextureWidth = bakedDataInfos.scatteringTextureWidth;
	m_TextureInfoData.scatteringTextureHeight = bakedDataInfos.scatteringTextureHeight;
	m_TextureInfoData.scatteringTextureDepth = bakedDataInfos.scatteringTextureDepth;
	m_TextureInfoData.irrandianceTextureWidth = bakedDataInfos.irrandianceTextureWidth;
	m_TextureInfoData.irrandianceTextureHeight = bakedDataInfos.irrandianceTextureHeight;

	{
		insigne::ubdesc_t desc;
		desc.region_size = floral::next_pow2(size(sizeof(TextureInfoData)));
		desc.data = &m_TextureInfoData;
		desc.data_size = sizeof(TextureInfoData);
		desc.usage = insigne::buffer_usage_e::static_draw;
		m_TextureInfoUB = insigne::copy_create_ub(desc);
		insigne::helpers::assign_uniform_block(m_MSPair.material, "ub_TextureInfo", 0, 0, m_TextureInfoUB);
		insigne::helpers::assign_uniform_block(m_SphereMSPair.material, "ub_TextureInfo", 0, 0, m_TextureInfoUB);
		insigne::helpers::assign_uniform_block(m_HelmetMSPair.material, "ub_TextureInfo", 0, 0, m_TextureInfoUB);
	}

	m_AtmosphereData.solarIrradiance = floral::vec4f(m_SkyFixedConfigs.solarIrradiance, 0.0f);
	m_AtmosphereData.rayleighScattering = floral::vec4f(m_SkyFixedConfigs.rayleighScattering, 0.0f);
	m_AtmosphereData.mieScattering = m_SkyFixedConfigs.mieScattering;
	m_AtmosphereData.sunAngularRadius = m_SkyFixedConfigs.sunAngularRadius;
	m_AtmosphereData.bottomRadius = m_SkyFixedConfigs.bottomRadius;
	m_AtmosphereData.topRadius = m_SkyFixedConfigs.topRadius;
	m_AtmosphereData.miePhaseFunctionG = m_SkyFixedConfigs.miePhaseFunctionG;
	m_AtmosphereData.muSMin = m_SkyFixedConfigs.muSMin;

	{
		insigne::ubdesc_t desc;
		desc.region_size = floral::next_pow2(size(sizeof(AtmosphereData)));
		desc.data = &m_AtmosphereData;
		desc.data_size = sizeof(AtmosphereData);
		desc.usage = insigne::buffer_usage_e::static_draw;
		m_AtmosphereUB = insigne::copy_create_ub(desc);
		insigne::helpers::assign_uniform_block(m_MSPair.material, "ub_Atmosphere", 0, 0, m_AtmosphereUB);
		insigne::helpers::assign_uniform_block(m_SphereMSPair.material, "ub_Atmosphere", 0, 0, m_AtmosphereUB);
		insigne::helpers::assign_uniform_block(m_HelmetMSPair.material, "ub_Atmosphere", 0, 0, m_AtmosphereUB);
	}

	{
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		insigne::color_attachment_t atm;
		strcpy(atm.name, "pre_convolution_fb");
		atm.texture_format = insigne::texture_format_e::hdr_rgb;
		atm.texture_dimension = insigne::texture_dimension_e::tex_cube;
		desc.color_attachments->push_back(atm);
		desc.width = 512; desc.height = 512;
		desc.has_depth = false;
		desc.color_has_mipmap = false;

		m_PreConvoFB = insigne::create_framebuffer(desc);
		insigne::helpers::assign_texture(m_ConvoMSPair.material, "u_EnvMap",
				insigne::extract_color_attachment(m_PreConvoFB, 0));
	}

	{
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		insigne::color_attachment_t atm;
		strcpy(atm.name, "convolution_fb");
		atm.texture_format = insigne::texture_format_e::hdr_rgb;
		atm.texture_dimension = insigne::texture_dimension_e::tex_cube;
		desc.color_attachments->push_back(atm);
		desc.width = 256; desc.height = 256;
		desc.has_depth = false;
		desc.color_has_mipmap = true;

		m_ConvolutionFB = insigne::create_framebuffer(desc);
		insigne::helpers::assign_texture(m_SphereMSPair.material, "u_EnvMap",
				insigne::extract_color_attachment(m_ConvolutionFB, 0));
		insigne::helpers::assign_texture(m_HelmetMSPair.material, "u_PMREMTex",
				insigne::extract_color_attachment(m_ConvolutionFB, 0));
	}

	m_PreConvoDone = false;
	m_ConvoDone = false;
}

//-------------------------------------------------------------------

void SkyRuntime::_OnUpdate(const f32 i_deltaMs)
{
	bool viewConfigChanged = false;
	bool sunConfigChanged = false;
	bool projectionConfigChanged = false;
	ImGui::Begin("Controller##SkyRuntime");
	if (ImGui::SliderFloat("Sun Azimuth", &m_SunAzimuth, 0.0f, 2.0f * floral::pi, "%.2f"))
	{
		sunConfigChanged = true;
	}
	if (ImGui::SliderFloat("Sun Zenith", &m_SunZenith, 0.0, floral::pi, "%.2f"))
	{
		sunConfigChanged = true;
	}
	if (DebugVec3f("Look-at", &m_LookAt, "%4.2f"))
	{
		viewConfigChanged = true;
	}
	if (DebugVec3f("Campos", &m_CamPos, "%4.2f"))
	{
		viewConfigChanged = true;
	}
	if (ImGui::SliderFloat("FovY", &m_FovY, 1.0f, 100.0f, "%.2f"))
	{
		projectionConfigChanged = true;
	}
	DebugMat4fRowOrder("ModelFromView", &m_SceneData.modelFromView);
	DebugMat4fRowOrder("ViewFromClip", &m_SceneData.viewFromClip);
	ImGui::End();

	if (viewConfigChanged || projectionConfigChanged)
	{
		m_ProjectionMatrix = floral::construct_infinity_perspective(0.01f, m_FovY, m_AspectRatio);
		floral::mat4x4f v = floral::construct_lookat_point(
				floral::vec3f(0.0f, 0.0f, 1.0f),
				m_CamPos, m_LookAt);
		m_ViewProjectionMatrix = m_ProjectionMatrix * v;
		m_SceneData.viewFromClip = m_ProjectionMatrix.get_inverse();
		m_SceneData.modelFromView = v.get_inverse();
		insigne::copy_update_ub(m_SceneUB, &m_SceneData, sizeof(SceneData), 0);
		m_ConfigsData.camera = floral::vec4f(m_CamPos, 1.0f);

		m_ObjectSceneData.viewProjectionMatrix = m_ViewProjectionMatrix;
		m_ObjectSceneData.cameraPosition = floral::vec4f(m_CamPos, 1.0f);
		insigne::copy_update_ub(m_ObjectSceneUB, &m_ObjectSceneData, sizeof(ObjectSceneData), 0);
	}

	if (sunConfigChanged || viewConfigChanged)
	{
		m_ConfigsData.sunDirection = floral::vec4f(
				cosf(m_SunAzimuth) * sinf(m_SunZenith),
				sinf(m_SunAzimuth) * sinf(m_SunZenith),
				cosf(m_SunZenith), 0.0f);
		insigne::copy_update_ub(m_ConfigsUB, &m_ConfigsData, sizeof(ConfigsData), 0);
		m_PreConvoDone = false;
		m_ConvoDone = false;
	}

	s32 m_GridRange = 10;
	f32 m_GridSpacing = 1.0f;
	const f32 maxCoord = m_GridRange * m_GridSpacing;
	for (s32 i = -m_GridRange; i <= m_GridRange; i++)
	{
		debugdraw::DrawLine3D(floral::vec3f(i * m_GridSpacing, -maxCoord, 0.0f),
				floral::vec3f(i * m_GridSpacing, maxCoord, 0.0f),
				floral::vec4f(0.3f, 0.3f, 0.3f, 0.3f));
		debugdraw::DrawLine3D(floral::vec3f(-maxCoord, i * m_GridSpacing, 0.0f),
				floral::vec3f(maxCoord, i * m_GridSpacing, 0.0f),
				floral::vec4f(0.3f, 0.3f, 0.3f, 0.3f));
	}

	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(1.0f, 0.0f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 1.0f, 0.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 0.0f, 1.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
}

//-------------------------------------------------------------------

void SkyRuntime::_OnRender(const f32 i_deltaMs)
{
	static const floral::vec3f s_lookAt[] =
	{
		floral::vec3f(1.0f, 0.0f, 0.0f),
		floral::vec3f(-1.0f, 0.0f, 0.0f),
		floral::vec3f(0.0f, 1.0f, 0.0f),
		floral::vec3f(0.0f, -1.0f, 0.0f),
		floral::vec3f(0.0f, 0.0f, 1.0f),
		floral::vec3f(0.0f, 0.0f, -1.0f),
	};

	static const floral::vec3f s_upDir[] =
	{
		floral::vec3f(0.0f, -1.0f, 0.0f),
		floral::vec3f(0.0f, -1.0f, 0.0f),
		floral::vec3f(0.0f, 0.0f, 1.0f),
		floral::vec3f(0.0f, 0.0f, -1.0f),
		floral::vec3f(0.0f, -1.0f, 0.0f),
		floral::vec3f(0.0f, -1.0f, 0.0f),
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

	if (!m_PreConvoDone)
	{
		floral::mat4x4f oldInvP = m_SceneData.viewFromClip;
		floral::mat4x4f oldInvV = m_SceneData.modelFromView;

		floral::mat4x4f p = floral::construct_infinity_perspective(0.01f, 90.0f, 1.0f);
		m_SceneData.viewFromClip = p.get_inverse();
		m_ConfigsData.camera = floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f);
		insigne::copy_update_ub(m_ConfigsUB, &m_ConfigsData, sizeof(ConfigsData), 0);

		for (int i = 0; i < 6; i++)
		{
			floral::mat4x4f v = floral::construct_lookat_dir(s_upDir[i], floral::vec3f(0.0f, 0.0f, 1.0f), s_lookAt[i]);
			m_SceneData.modelFromView = v.get_inverse();
			insigne::copy_update_ub(m_SceneUB, &m_SceneData, sizeof(SceneData), 0);

			insigne::begin_render_pass(m_PreConvoFB, s_faces[i]);
			insigne::draw_surface<geo2d::SurfacePT>(m_Quad.vb, m_Quad.ib, m_MSPair.material);
			insigne::end_render_pass(m_PreConvoFB);
			insigne::dispatch_render_pass();
		}

		m_ConfigsData.camera = floral::vec4f(m_CamPos, 1.0f);
		insigne::copy_update_ub(m_ConfigsUB, &m_ConfigsData, sizeof(ConfigsData), 0);
		m_SceneData.viewFromClip = oldInvP;
		m_SceneData.modelFromView = oldInvV;
		insigne::copy_update_ub(m_SceneUB, &m_SceneData, sizeof(SceneData), 0);
		m_PreConvoDone = true;
	}

	if (m_PreConvoDone && !m_ConvoDone)
	{
		for (s32 m = 0; m < 9; m++)
		{
			f32 r = (f32)m / 8.0f;
			m_ConvolutionSceneData.roughness = r;
			for (int i = 0; i < 6; i++)
			{
				floral::mat4x4f p = floral::construct_infinity_perspective(0.01f, 90.0f, 1.0f);
				floral::mat4x4f v = floral::construct_lookat_dir(s_upDir[i], floral::vec3f(0.0f, 0.0f, 0.0f), s_lookAt[i]);
				m_ConvolutionSceneData.viewProjectionMatrix = p * v;
				insigne::copy_update_ub(m_ConvolutionUB, &m_ConvolutionSceneData, sizeof(ConvolutionSceneData), 0);

				insigne::begin_render_pass(m_ConvolutionFB, s_faces[i], m);
				insigne::draw_surface<geo3d::SurfacePNC>(m_Surface.vb, m_Surface.ib, m_ConvoMSPair.material);
				insigne::end_render_pass(m_ConvolutionFB);
				insigne::dispatch_render_pass();
			}
		}
		m_ConvoDone = true;
	}

	m_PostFXChain.BeginMainOutput();
	insigne::draw_surface<geo2d::SurfacePT>(m_Quad.vb, m_Quad.ib, m_MSPair.material);
	insigne::draw_surface<geo3d::SurfacePNTT>(m_HelmetSurfaceGPU.vb, m_HelmetSurfaceGPU.ib, m_HelmetMSPair.material, "helmet");
	//insigne::draw_surface<geo3d::SurfacePNC>(m_Surface.vb, m_Surface.ib, m_SphereMSPair.material);
	debugdraw::Render(m_ViewProjectionMatrix);
	m_PostFXChain.EndMainOutput();

	m_PostFXChain.Process();

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	m_PostFXChain.Present();
	RenderImGui();
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

//-------------------------------------------------------------------

void SkyRuntime::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	insigne::unregister_surface_type<geo3d::SurfacePNC>();
	insigne::unregister_surface_type<geo3d::SurfacePNTT>();
	insigne::unregister_surface_type<geo2d::SurfacePT>();

	g_MemoryManager.destroy_allocator(m_TexDataArenaRegion);

	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_ModelDataArena);
	g_StreammingAllocator.free(m_PostFXArena);
	g_StreammingAllocator.free(m_MemoryArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------

insigne::texture_handle_t SkyRuntime::LoadRawHDRTexture2D(const_cstr i_texFile, const s32 i_w, const s32 i_h, const s32 i_channel)
{
	floral::relative_path iFilePath = floral::build_relative_path(i_texFile);
	floral::file_info iFile = floral::open_file_read(m_FileSystem, iFilePath);
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	FLORAL_ASSERT(iFile.file_size == sizeBytes);
	f32* texData = (f32*)m_TexDataArena.allocate(sizeBytes);
	floral::read_all_file(iFile, texData);
	floral::close_file(iFile);

	insigne::texture_desc_t texDesc;
	texDesc.width = i_w;
	texDesc.height = i_h;
	texDesc.format = insigne::texture_format_e::hdr_rgb_high;
	texDesc.min_filter = insigne::filtering_e::linear;
	texDesc.mag_filter = insigne::filtering_e::linear;
	texDesc.dimension = insigne::texture_dimension_e::tex_2d;
	texDesc.has_mipmap = false;
	texDesc.data = texData;

	return insigne::create_texture(texDesc);
}

//-------------------------------------------------------------------

insigne::texture_handle_t SkyRuntime::LoadRawHDRTexture3D(const_cstr i_texFile, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel)
{
	floral::relative_path iFilePath = floral::build_relative_path(i_texFile);
	floral::file_info iFile = floral::open_file_read(m_FileSystem, iFilePath);
	ssize sizeBytes = i_w * i_h * i_d * i_channel * sizeof(f32);
	FLORAL_ASSERT(iFile.file_size == sizeBytes);
	f32* texData = (f32*)m_TexDataArena.allocate(sizeBytes);
	floral::read_all_file(iFile, texData);
	floral::close_file(iFile);

	insigne::texture_desc_t texDesc;
	texDesc.width = i_w;
	texDesc.height = i_h;
	texDesc.depth = i_d;
	if (i_channel == 3)
	{
		texDesc.format = insigne::texture_format_e::hdr_rgb_high;
	}
	else if (i_channel == 4)
	{
		texDesc.format = insigne::texture_format_e::hdr_rgba_high;
	}
	else
	{
		FLORAL_ASSERT(false);
	}
	texDesc.min_filter = insigne::filtering_e::linear;
	texDesc.mag_filter = insigne::filtering_e::linear;
	texDesc.wrap_s = insigne::wrap_e::clamp_to_edge;
	texDesc.wrap_t = insigne::wrap_e::clamp_to_edge;
	texDesc.wrap_r = insigne::wrap_e::clamp_to_edge;
	texDesc.dimension = insigne::texture_dimension_e::tex_3d;
	texDesc.has_mipmap = false;
	texDesc.data = texData;

	return insigne::create_texture(texDesc);
}

//-------------------------------------------------------------------
}
}
