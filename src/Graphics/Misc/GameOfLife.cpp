#include "GameOfLife.h"

#include <clover/Logger.h>

#include <calyx/context.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>

#include "InsigneImGui.h"

#include "Graphics/PostFXParser.h"
#include "Graphics/MaterialParser.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone
{
namespace misc
{
//-------------------------------------------------------------------

const bool ReadBoardMirroredRepeat(bool* i_board, s32 i_x, s32 i_y, const s32 i_size)
{
	FLORAL_ASSERT(i_x >= -1 && i_x <= i_size);
	FLORAL_ASSERT(i_y >= -1 && i_y <= i_size);
	if (i_x == -1) i_x = i_size - 1;
	if (i_y == -1) i_y = i_size - 1;
	if (i_x == i_size) i_x = 0;
	if (i_y == i_size) i_y = 0;

	if (i_x >= 0 && i_y >= 0 && i_x < i_size && i_y < i_size)
	{
		return i_board[i_y * i_size + i_x];
	}
	else
	{
		FLORAL_ASSERT(false);
		return false;
	}
}

void UpdateGameOfLife(bool* o_front, bool* i_back, const s32 i_size)
{
	for (s32 y = 0; y < i_size; y++)
	{
		for (s32 x = 0; x < i_size; x++)
		{
			const bool cellStatus = ReadBoardMirroredRepeat(i_back, x, y, i_size);
			s32 aliveNeighbors = 0;
			if (ReadBoardMirroredRepeat(i_back, x - 1, y, i_size))
			{
				aliveNeighbors++;
			}
			if (ReadBoardMirroredRepeat(i_back, x, y - 1, i_size))
			{
				aliveNeighbors++;
			}
			if (ReadBoardMirroredRepeat(i_back, x + 1, y, i_size))
			{
				aliveNeighbors++;
			}
			if (ReadBoardMirroredRepeat(i_back, x, y + 1, i_size))
			{
				aliveNeighbors++;
			}
			if (ReadBoardMirroredRepeat(i_back, x - 1, y - 1, i_size))
			{
				aliveNeighbors++;
			}
			if (ReadBoardMirroredRepeat(i_back, x + 1, y - 1, i_size))
			{
				aliveNeighbors++;
			}
			if (ReadBoardMirroredRepeat(i_back, x - 1, y + 1, i_size))
			{
				aliveNeighbors++;
			}
			if (ReadBoardMirroredRepeat(i_back, x + 1, y + 1, i_size))
			{
				aliveNeighbors++;
			}

			if (cellStatus) // alive
			{
				if (aliveNeighbors == 2 || aliveNeighbors == 3) // survive
				{
					o_front[y * i_size + x] = true;
				}
				else // die
				{
					o_front[y * i_size + x] = false;
				}
			}
			else // dead
			{
				if (aliveNeighbors == 3)
				{
					o_front[y * i_size + x] = true;
				}
				else
				{
					o_front[y * i_size + x] = false;
				}
			}
		}
	}
}

//-------------------------------------------------------------------

GameOfLife::GameOfLife()
{
}

GameOfLife::~GameOfLife()
{
}

ICameraMotion* GameOfLife::GetCameraMotion()
{
	return nullptr;
}

const_cstr GameOfLife::GetName() const
{
	return k_name;
}

void GameOfLife::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);

	floral::relative_path wdir = floral::build_relative_path("tests/misc/game_of_life");
	floral::push_directory(m_FileSystem, wdir);

	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(2));
	m_ResourceArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(16));
	m_PostFXArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(128));

	// register surfaces
	insigne::register_surface_type<geo2d::SurfacePT>();

	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
	const f32 k_aspectRatio = (f32)commonCtx->window_width / (f32)commonCtx->window_height;
	const f32 k_width = 0.5f;
	const f32 k_height = k_width * k_aspectRatio;
	floral::inplace_array<geo2d::VertexPT, 4> vertices;
	vertices.push_back({ { -k_width, k_height }, { 0.0f, 1.0f } });
	vertices.push_back({ { -k_width, -k_height }, { 0.0f, 0.0f } });
	vertices.push_back({ { k_width, -k_height }, { 1.0f, 0.0f } });
	vertices.push_back({ { k_width, k_height }, { 1.0f, 1.0f } });

	floral::inplace_array<s32, 6> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(0);

	m_Quad = helpers::CreateSurfaceGPU(&vertices[0], 4, sizeof(geo2d::VertexPT),
			&indices[0], 6, insigne::buffer_usage_e::static_draw, true);

	m_TexData = m_ResourceArena->allocate_array<floral::vec3f>(128 * 128);
	m_FrontLifeBuffer = m_ResourceArena->allocate_array<bool>(128 * 128);
	m_BackLifeBuffer = m_ResourceArena->allocate_array<bool>(128 * 128);

	memset(m_FrontLifeBuffer, 0, 128 * 128 * sizeof(bool));
	memset(m_BackLifeBuffer, 0, 128 * 128 * sizeof(bool));

	// read from file
	m_MemoryArena->free_all();
	floral::relative_path inputPath = floral::build_relative_path("initial.board");
	floral::file_info inp = floral::open_file_read(m_FileSystem, inputPath);
	floral::file_stream inpStream;
	inpStream.buffer = (p8)m_MemoryArena->allocate(inp.file_size);
	floral::read_all_file(inp, inpStream);
	floral::close_file(inp);
	for (s32 y = 0; y < 128; y++)
	{
		for (s32 x = 0; x < 128; x++)
		{
			c8 cellStatus = 0;
			do
			{
				cellStatus = inpStream.read_char();
			} while (cellStatus != '0' && cellStatus != '1');

			if (cellStatus == '1')
			{
				m_BackLifeBuffer[y * 128 + x] = true;
			}
			else if (cellStatus == '0')
			{
				m_BackLifeBuffer[y * 128 + x] = false;
			}
			else
			{
				FLORAL_ASSERT(false);
			}
		}
	}

	insigne::texture_desc_t texDesc;
	texDesc.width = 128;
	texDesc.height = 128;
	texDesc.format = insigne::texture_format_e::hdr_rgb_high;
	texDesc.min_filter = insigne::filtering_e::nearest;
	texDesc.mag_filter = insigne::filtering_e::nearest;
	texDesc.dimension = insigne::texture_dimension_e::tex_2d;
	texDesc.wrap_s = insigne::wrap_e::clamp_to_edge;
	texDesc.wrap_t = insigne::wrap_e::clamp_to_edge;
	texDesc.compression = insigne::texture_compression_e::no_compression;
	texDesc.has_mipmap = false;
	texDesc.data = nullptr;

	m_Texture = insigne::create_texture(texDesc);

	const size dataSize = insigne::prepare_texture_desc(texDesc);
	insigne::copy_update_texture(m_Texture, m_TexData, dataSize);
#if 0
	insigne::copy_update_texture(m_Texture, m_TexData, 128 * 128 * sizeof(floral::vec3f));
#endif

	m_MemoryArena->free_all();
	floral::relative_path matPath = floral::build_relative_path("main.mat");
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	const bool createResult = mat_loader::CreateMaterial(&m_MSPair, m_FileSystem, matDesc, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);
	insigne::helpers::assign_texture(m_MSPair.material, "u_MainTex", m_Texture);

	m_MemoryArena->free_all();
	floral::relative_path pfxPath = floral::build_relative_path("postfx.pfx");
	pfx_parser::PostEffectsDescription pfxDesc = pfx_parser::ParsePostFX(m_FileSystem, pfxPath, m_MemoryArena);
	m_PostFXChain.Initialize(m_FileSystem, pfxDesc, floral::vec2f(commonCtx->window_width, commonCtx->window_height), m_PostFXArena);
}

void GameOfLife::_OnUpdate(const f32 i_deltaMs)
{
	static size frameIdx = 0;
	frameIdx++;
	if (frameIdx % 5 == 0)
	{
		UpdateGameOfLife(m_FrontLifeBuffer, m_BackLifeBuffer, 128);
		bool* tmp = m_BackLifeBuffer;
		m_BackLifeBuffer = m_FrontLifeBuffer;
		m_FrontLifeBuffer = tmp;

		for (s32 y = 0; y < 128; y++)
		{
			for (s32 x = 0; x < 128; x++)
			{
				if (m_FrontLifeBuffer[y * 128 + x])
				{
					m_TexData[(127 - y) * 128 + x] = floral::vec3f(4.0f, 1.0f, 1.0f);
				}
				else
				{
					m_TexData[(127 - y) * 128 + x] = floral::vec3f(0.0f, 0.0, 0.0);
				}
			}
		}
		insigne::copy_update_texture(m_Texture, m_TexData, 128 * 128 * sizeof(floral::vec3f));
	}
}

void GameOfLife::_OnRender(const f32 i_deltaMs)
{
	m_PostFXChain.BeginMainOutput();
	insigne::draw_surface<geo2d::SurfacePT>(m_Quad.vb, m_Quad.ib, m_MSPair.material);
	m_PostFXChain.EndMainOutput();

	m_PostFXChain.Process();

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	m_PostFXChain.Present();
	RenderImGui();
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void GameOfLife::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	m_PostFXChain.CleanUp();

	insigne::unregister_surface_type<geo2d::SurfacePT>();

	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_PostFXArena);
	g_StreammingAllocator.free(m_ResourceArena);
	g_StreammingAllocator.free(m_MemoryArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------
}
}
