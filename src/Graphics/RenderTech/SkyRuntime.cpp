#include "SkyRuntime.h"

#include <calyx/context.h>

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

namespace stone
{
namespace tech
{
//-------------------------------------------------------------------

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
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(256));

	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
	f32 aspectRatio = (f32)commonCtx->window_width / (f32)commonCtx->window_height;

	// register surfaces
	insigne::register_surface_type<geo2d::SurfacePT>();

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

	m_TransmittanceTexture = LoadRawHDRTexture2D("transmittance_texture.rtex2d",
			bakedDataInfos.transmittanceTextureWidth, bakedDataInfos.transmittanceTextureHeight, 3);
	m_ScatteringTexture = LoadRawHDRTexture3D("scattering_texture.rtex3d",
			bakedDataInfos.scatteringTextureWidth, bakedDataInfos.scatteringTextureHeight, bakedDataInfos.scatteringTextureDepth, 4);
	m_IrradianceTexture = LoadRawHDRTexture2D("irradiance_texture.rtex2d",
			bakedDataInfos.irrandianceTextureWidth, bakedDataInfos.irrandianceTextureHeight, 3);

	insigne::helpers::assign_texture(m_MSPair.material, "u_TransmittanceTex", m_TransmittanceTexture);
	insigne::helpers::assign_texture(m_MSPair.material, "u_ScatteringTex", m_ScatteringTexture);
	insigne::helpers::assign_texture(m_MSPair.material, "u_IrradianceTex", m_IrradianceTexture);

	const f32 k_FovY = floral::to_radians(50.0f);
	const f32 k_tanFovY = tanf(k_FovY / 2.0f);
	m_SceneData.viewFromClip = floral::mat4x4f(
			floral::vec4f(k_tanFovY * aspectRatio, 0.0f, 0.0f, 0.0f),
			floral::vec4f(0.0f, k_tanFovY, 0.0f, 0.0f),
			floral::vec4f(0.0f, 0.0f, 0.0f, -1.0f),
			floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f)).get_transpose();
	m_ViewDistance = 150.0f;
	m_ViewZenith = 1.47f;
	m_ViewAzimuth = 0.0f;
	f32 cosZ = cosf(m_ViewZenith);
	f32 sinZ = sinf(m_ViewZenith);
	f32 cosA = cosf(m_ViewAzimuth);
	f32 sinA = sinf(m_ViewAzimuth);
	floral::vec3f ux(-sinA, cosA, 0.0f);
	floral::vec3f uy(-cosZ * cosA, -cosZ * sinA, sinZ);
	floral::vec3f uz(sinZ * cosA, sinZ * sinA, cosZ);
	f32 l = m_ViewDistance / m_SkyFixedConfigs.unitLengthInMeters;
	// TODO: is this the inverse of the look-at matrix (?)
	m_SceneData.modelFromView = floral::mat4x4f(
			floral::vec4f(ux.x, uy.x, uz.x, /*uz.x * l*/0.0f),
			floral::vec4f(ux.y, uy.y, uz.y, /*uz.y * l*/0.0f),
			floral::vec4f(ux.z, uy.z, uz.z, /*uz.z * l*/l),
			floral::vec4f(0.0f, 0.0f, 0.0f, 1.0f)).get_transpose();
	{
		insigne::ubdesc_t desc;
		desc.region_size = floral::next_pow2(size(sizeof(SceneData)));
		desc.data = &m_SceneData;
		desc.data_size = sizeof(SceneData);
		desc.usage = insigne::buffer_usage_e::dynamic_draw;
		m_SceneUB = insigne::copy_create_ub(desc);
		insigne::helpers::assign_uniform_block(m_MSPair.material, "ub_Scene", 0, 0, m_SceneUB);
	}

	m_SunZenith = 1.564f;
	m_SunAzimuth = -3.0f;
	//m_ConfigsData.camera = floral::vec4f(uz.x * l, uz.y * l, uz.z * l, 1.0f);
	m_ConfigsData.camera = floral::vec4f(0.0f, 0.0f, l, 1.0f);
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
	}
}

//-------------------------------------------------------------------

void SkyRuntime::_OnUpdate(const f32 i_deltaMs)
{
	bool viewConfigChanged = false;
	bool sunConfigChanged = false;
	ImGui::Begin("Controller##SkyRuntime");
	if (ImGui::SliderFloat("View Azimuth", &m_ViewAzimuth, 0.0f, 2.0f * floral::pi, "%.2f"))
	{
		viewConfigChanged = true;
	}
	if (ImGui::SliderFloat("View Zenith", &m_ViewZenith, 0.0f, floral::pi, "%.2f"))
	{
		viewConfigChanged = true;
	}
	if (ImGui::SliderFloat("View Distance (m)", &m_ViewDistance, 0.0f, 1000.0f, "%.2f"))
	{
		viewConfigChanged = true;
	}
	if (ImGui::SliderFloat("Sun Azimuth", &m_SunAzimuth, 0.0f, 2.0f * floral::pi, "%.2f"))
	{
		sunConfigChanged = true;
	}
	if (ImGui::SliderFloat("Sun Zenith", &m_SunZenith, 0.0, floral::pi, "%.2f"))
	{
		sunConfigChanged = true;
	}
	DebugMat4fRowOrder("ModelFromView", &m_SceneData.modelFromView);
	DebugMat4fRowOrder("ViewFromClip", &m_SceneData.viewFromClip);
	ImGui::End();

	if (viewConfigChanged)
	{
		f32 cosZ = cosf(m_ViewZenith);
		f32 sinZ = sinf(m_ViewZenith);
		f32 cosA = cosf(m_ViewAzimuth);
		f32 sinA = sinf(m_ViewAzimuth);
		floral::vec3f ux(-sinA, cosA, 0.0f);
		floral::vec3f uy(-cosZ * cosA, -cosZ * sinA, sinZ);
		floral::vec3f uz(sinZ * cosA, sinZ * sinA, cosZ);
		f32 l = m_ViewDistance / m_SkyFixedConfigs.unitLengthInMeters;
		m_SceneData.modelFromView = floral::mat4x4f(
				floral::vec4f(ux.x, uy.x, uz.x, /*uz.x * l*/0.0f),
				floral::vec4f(ux.y, uy.y, uz.y, /*uz.y * l*/0.0f),
				floral::vec4f(ux.z, uy.z, uz.z, /*uz.z * l*/l),
				floral::vec4f(0.0f, 0.0f, 0.0f, 1.0f)).get_transpose();
		insigne::copy_update_ub(m_SceneUB, &m_SceneData, sizeof(SceneData), 0);
		//m_ConfigsData.camera = floral::vec4f(uz.x * l, uz.y * l, uz.z * l, 1.0f);
	}

	if (sunConfigChanged || viewConfigChanged)
	{
		m_ConfigsData.sunDirection = floral::vec4f(
				cosf(m_SunAzimuth) * sinf(m_SunZenith),
				sinf(m_SunAzimuth) * sinf(m_SunZenith),
				cosf(m_SunZenith), 0.0f);
		insigne::copy_update_ub(m_ConfigsUB, &m_ConfigsData, sizeof(ConfigsData), 0);
	}
}

//-------------------------------------------------------------------

void SkyRuntime::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<geo2d::SurfacePT>(m_Quad.vb, m_Quad.ib, m_MSPair.material);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

//-------------------------------------------------------------------

void SkyRuntime::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	insigne::unregister_surface_type<geo2d::SurfacePT>();

	g_MemoryManager.destroy_allocator(m_TexDataArenaRegion);

	g_StreammingAllocator.free(m_MaterialDataArena);
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
