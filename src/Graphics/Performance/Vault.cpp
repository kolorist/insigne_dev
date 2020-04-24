#include "Vault.h"

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include <clover/Logger.h>

#include <floral/math/utils.h>

#include "InsigneImGui.h"

#include "Graphics/MaterialParser.h"
#include "Graphics/TextureLoader.h"
#include "Graphics/GLTFLoader.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/CbModelLoader.h"
#include "Graphics/DebugDrawer.h"

namespace stone
{
namespace perf
{

static const_cstr k_SuiteName = "test_vault";

Vault::Vault()
{
}

Vault::~Vault()
{
}

ICameraMotion* Vault::GetCameraMotion()
{
	return nullptr;
}

const_cstr Vault::GetName() const
{
	return k_name;
}

floral::mat4x4f CreateViewMatrixLH(const floral::vec3f& i_position, const floral::vec3f& i_lookAt, const floral::vec3f& i_up)
{
	floral::vec3f front = floral::normalize(i_lookAt - i_position);
	floral::vec3f side = floral::normalize(floral::cross(i_up, front));
	floral::vec3f up = floral::cross(front, side);

	floral::mat4x4f rotateM(1.0f);
	rotateM[0][0] = side.x;
	rotateM[1][0] = side.y;
	rotateM[2][0] = side.z;

	rotateM[0][1] = up.x;
	rotateM[1][1] = up.y;
	rotateM[2][1] = up.z;

	rotateM[0][2] = front.x;
	rotateM[1][2] = front.y;
	rotateM[2][2] = front.z;

	floral::mat4x4f translateM(1.0f);
	translateM[3][0] = -i_position.x;
	translateM[3][1] = -i_position.y;
	translateM[3][2] = -i_position.z;

	floral::mat4x4f m = rotateM * translateM;
	return m;
}

floral::mat4x4f CreateViewMatrixRH(const floral::vec3f& i_position, const floral::vec3f& i_lookAt, const floral::vec3f& i_up)
{
	floral::vec3f front = floral::normalize(i_lookAt - i_position);
	floral::vec3f side = floral::normalize(floral::cross(front, i_up));
	floral::vec3f up = floral::cross(side, front);

	floral::mat4x4f rotateM(1.0f);
	rotateM[0][0] = side.x;
	rotateM[1][0] = side.y;
	rotateM[2][0] = side.z;

	rotateM[0][1] = up.x;
	rotateM[1][1] = up.y;
	rotateM[2][1] = up.z;

	rotateM[0][2] = -front.x;
	rotateM[1][2] = -front.y;
	rotateM[2][2] = -front.z;

	floral::mat4x4f translateM(1.0f);
	translateM[3][0] = -i_position.x;
	translateM[3][1] = -i_position.y;
	translateM[3][2] = -i_position.z;

	floral::mat4x4f m = rotateM * translateM;
	return m;
}

floral::mat4x4f CreatePerspectiveLH(const f32 i_aspectRatio, const f32 i_fovy, const f32 i_near, const f32 i_far)
{
	const f32 tanHalfFovY = tanf(floral::to_radians(i_fovy / 2.0f));

	floral::mat4x4f m;
	m[0][0] = 1.0f / (i_aspectRatio * tanHalfFovY);
	m[1][1] = 1.0f / tanHalfFovY;
	m[2][2] = (i_far + i_near) / (i_far - i_near);
	m[2][3] = 1.0f;
	m[3][2] = -2.0f * i_far * i_near / (i_far - i_near);

	return m;
}

floral::mat4x4f CreatePerspectiveRH(const f32 i_aspectRatio, const f32 i_fovy, const f32 i_near, const f32 i_far)
{
	const f32 tanHalfFovY = tanf(floral::to_radians(i_fovy / 2.0f));

	floral::mat4x4f m;
	m[0][0] = 1.0f / (i_aspectRatio * tanHalfFovY);
	m[1][1] = 1.0f / tanHalfFovY;
	m[2][2] = -(i_far + i_near) / (i_far - i_near);
	m[2][3] = -1.0f;
	m[3][2] = -2.0f * i_far * i_near / (i_far - i_near);

	return m;
}

void Vault::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_SuiteName);
	// register surfaces
	insigne::register_surface_type<geo2d::SurfacePT>();
	insigne::register_surface_type<geo3d::SurfacePNT>();

	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(16));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(256));
	m_ModelDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(1));

	m_MemoryArena->free_all();
	cbmodel::Model<geo3d::VertexPNT> model = cbmodel::LoadModelData<geo3d::VertexPNT>(floral::path("gfx/go/models/demo/DamagedHelmet.gltf_mesh_0.cbmodel"),
			cbmodel::VertexAttribute::Position | cbmodel::VertexAttribute::Normal | cbmodel::VertexAttribute::TexCoord,
			m_MemoryArena, m_ModelDataArena);

	m_SurfaceGPU = helpers::CreateSurfaceGPU(model.verticesData, model.verticesCount, sizeof(geo3d::VertexPNT),
			model.indicesData, model.indicesCount, insigne::buffer_usage_e::static_draw, false);

	floral::inplace_array<geo2d::VertexPT, 4> ssVertices;
	ssVertices.push_back({ { -1.0f, -1.0f }, { 0.0f, 0.0f } });
	ssVertices.push_back({ { 1.0f, -1.0f }, { 1.0f, 0.0f } });
	ssVertices.push_back({ { 1.0f, 1.0f }, { 1.0f, 1.0f } });
	ssVertices.push_back({ { -1.0f, 1.0f }, { 0.0f, 1.0f } });

	floral::inplace_array<s32, 6> ssIndices;
	ssIndices.push_back(0);
	ssIndices.push_back(1);
	ssIndices.push_back(2);
	ssIndices.push_back(2);
	ssIndices.push_back(3);
	ssIndices.push_back(0);

	m_ScreenQuad = helpers::CreateSurfaceGPU(&ssVertices[0], 4, sizeof(geo2d::VertexPT),
			&ssIndices[0], 6, insigne::buffer_usage_e::static_draw, true);

	// load SH and PMREM
	{
		m_MemoryArena->free_all();
		floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/sunflowers.prb");
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);
		floral::close_file(texFile);

		floral::vec3f sh[9];
		dataStream.read(&sh);
		for (ssize i = 0; i < 9; i++)
		{
			m_SceneData.sh[i] = floral::vec4f(sh[i], 0.0f);
		}
		s32 faceSize = 0;
		dataStream.read(&faceSize);

		insigne::texture_desc_t demoTexDesc;
		demoTexDesc.width = faceSize;
		demoTexDesc.height = faceSize;
		demoTexDesc.format = insigne::texture_format_e::hdr_rgb;
		demoTexDesc.min_filter = insigne::filtering_e::linear_mipmap_linear;
		demoTexDesc.mag_filter = insigne::filtering_e::linear;
		demoTexDesc.dimension = insigne::texture_dimension_e::tex_cube;
		demoTexDesc.has_mipmap = true;
		const size dataSize = insigne::prepare_texture_desc(demoTexDesc);
		p8 pData = (p8)demoTexDesc.data;
		// > This is where it get *really* interesting
		// 	Totally opposite of normal 2D texture mapping, CubeMapping define the origin of the texture sampling coordinate
		// 	from the lower left corner. OmegaLUL
		// > Reason: historical reason (from Renderman)
		dataStream.read_bytes((p8)demoTexDesc.data, dataSize);

		m_CubeMapTex = insigne::create_texture(demoTexDesc);
	}

	// prebake split sum for BRDF
	{
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		insigne::color_attachment_t atm;
		strcpy(atm.name, "main_color");
		atm.texture_format = insigne::texture_format_e::hdr_rg;
		atm.texture_dimension = insigne::texture_dimension_e::tex_2d;
		desc.color_attachments->push_back(atm);
		desc.width = 512; desc.height = 512;
		desc.has_depth = false;

		m_BrdfFB = insigne::create_framebuffer(desc);
	}
	m_IsBakingSplitSum = true;

	//floral::mat4x4f view = CreateViewMatrixLH(floral::vec3f(3.0f, 3.0f, 3.0f),
	floral::mat4x4f view = CreateViewMatrixRH(floral::vec3f(2.0f, 2.0f, 2.0f),
			floral::vec3f(0.0f, 0.0f, 0.0f),
			floral::vec3f(0.0f, 0.0f, 1.0f));
	//floral::mat4x4f projection = CreatePerspectiveLH(16.0f / 9.0f, 45.0f, 0.01f, 100.0f);
	floral::mat4x4f projection = CreatePerspectiveRH(16.0f / 9.0f, 45.0f, 0.01f, 100.0f);
	m_SceneData.cameraPos = floral::vec4f(2.0f, 2.0f, 2.0f, 0.0f);
	m_SceneData.viewProjectionMatrix = projection * view;

	insigne::ubdesc_t desc;
	desc.region_size = floral::next_pow2(sizeof(SceneData));
	desc.data = &m_SceneData;
	desc.data_size = sizeof(SceneData);
	desc.usage = insigne::buffer_usage_e::dynamic_draw;
	m_SceneUB = insigne::copy_create_ub(desc);

	m_MemoryArena->free_all();
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(
			floral::path("gfx/mat/pbr.mat"), m_MemoryArena);

	const bool pbrMaterialResult = mat_loader::CreateMaterial(&m_MSPair, matDesc, m_MaterialDataArena);
	FLORAL_ASSERT(pbrMaterialResult == true);

	insigne::helpers::assign_uniform_block(m_MSPair.material, "ub_Scene", 0, 0, m_SceneUB);
	insigne::helpers::assign_texture(m_MSPair.material, "u_PMREM", m_CubeMapTex);
	insigne::helpers::assign_texture(m_MSPair.material, "u_SplitSum", insigne::extract_color_attachment(m_BrdfFB, 0));

	m_MemoryArena->free_all();
	matDesc = mat_parser::ParseMaterial(floral::path("gfx/mat/pbr_splitsum.mat"), m_MemoryArena);

	const bool ssMaterialResult = mat_loader::CreateMaterial(&m_SplitSumPair, matDesc, m_MaterialDataArena);
	FLORAL_ASSERT(ssMaterialResult == true);
}

void Vault::_OnUpdate(const f32 i_deltaMs)
{
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(2.0f, 0.0f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 2.0f, 0.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 0.0f, 2.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
}

void Vault::_OnRender(const f32 i_deltaMs)
{
	if (m_IsBakingSplitSum)
	{
		insigne::begin_render_pass(m_BrdfFB);
		insigne::draw_surface<geo2d::SurfacePT>(m_ScreenQuad.vb, m_ScreenQuad.ib, m_SplitSumPair.material);
		insigne::end_render_pass(m_BrdfFB);
		insigne::dispatch_render_pass();
		m_IsBakingSplitSum = false;
	}

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<geo3d::SurfacePNT>(m_SurfaceGPU.vb, m_SurfaceGPU.ib, m_MSPair.material);
	debugdraw::Render(m_SceneData.viewProjectionMatrix);
	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void Vault::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_SuiteName);
	g_StreammingAllocator.free(m_ModelDataArena);
	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_MemoryArena);
	insigne::unregister_surface_type<geo3d::SurfacePNT>();
	insigne::unregister_surface_type<geo2d::SurfacePT>();
}

}
}
