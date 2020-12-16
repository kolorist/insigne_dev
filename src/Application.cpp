#include "Application.h"

#include <floral.h>
#include <clover.h>
#include <lotus/events.h>
#include <lotus/profiler.h>
#include <insigne/configs.h>
#include <insigne/system.h>
#include <insigne/driver.h>
#include <insigne/ut_render.h>
#include <calyx/context.h>

#include "System/Controller.h"

#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/ICameraMotion.h"
#include "Graphics/IDebugUI.h"

#include "DemoHub.h"

namespace stone
{

Application::Application(Controller* i_controller)
	: m_Initialized(false)
	, m_DemoHub(nullptr)
{
	//s_profileEvents[0].init(4096u, &g_SystemAllocator);
	//s_profileEvents[1].init(4096u, &g_SystemAllocator);

	i_controller->IOEvents.OnInitializePlatform.bind<Application, &Application::OnInitializePlatform>(this);
	i_controller->IOEvents.OnInitializeRenderer.bind<Application, &Application::OnInitializeRenderer>(this);
	i_controller->IOEvents.OnInitializeGame.bind<Application, &Application::OnInitializeGame>(this);
	i_controller->IOEvents.OnPause.bind<Application, &Application::OnPause>(this);
	i_controller->IOEvents.OnResume.bind<Application, &Application::OnResume>(this);
	i_controller->IOEvents.OnFrameStep.bind<Application, &Application::OnFrameStep>(this);
	i_controller->IOEvents.OnCleanUp.bind<Application, &Application::OnCleanUp>(this);
	i_controller->IOEvents.CharacterInput.bind<Application, &Application::OnCharacterInput>(this);
	i_controller->IOEvents.KeyInput.bind<Application, &Application::OnKeyInput>(this);
	i_controller->IOEvents.CursorMove.bind<Application, &Application::OnCursorMove>(this);
	i_controller->IOEvents.CursorInteract.bind<Application, &Application::OnCursorInteract>(this);
}

Application::~Application()
{
	// TODO
}

// -----------------------------------------
void Application::UpdateFrame(f32 i_deltaMs)
{
	PROFILE_SCOPE("UpdateFrame");
	m_DemoHub->UpdateFrame(i_deltaMs);
	/*
	s_profileEvents[0].empty();
	lotus::unpack_capture(s_profileEvents[0], 0);
	s_profileEvents[1].empty();
	lotus::unpack_capture(s_profileEvents[1], 1);
	*/
}

void Application::RenderFrame(f32 i_deltaMs)
{
	PROFILE_SCOPE("RenderFrame");
	m_DemoHub->RenderFrame(i_deltaMs);
}

// -----------------------------------------
void Application::OnPause()
{
	// pause render thread
	LOG_TOPIC("app");
	CLOVER_VERBOSE("Pausing Render thread");
	insigne::pause_render_thread();
}

void Application::OnResume()
{
	// resume render thread
	LOG_TOPIC("app");
	CLOVER_VERBOSE("Resuming Render thread");
	insigne::resume_render_thread();
}

void Application::OnFocusChanged(bool i_hasFocus)
{
}

void Application::OnDisplayChanged()
{
}

void Application::OnInitializePlatform()
{
	LOG_TOPIC("app");
	CLOVER_VERBOSE("Initialize platform settings");
	// insigne settings
	insigne::g_settings.frame_shader_allocator_size_mb = 4;
	insigne::g_settings.frame_buffers_allocator_size_mb = 16;
	insigne::g_settings.frame_textures_allocator_size_mb = 48;
	insigne::g_settings.frame_render_allocator_size_mb = 4;
	insigne::g_settings.frame_draw_allocator_size_mb = 4;

	insigne::g_settings.draw_cmdbuff_arena_size_mb = 1;
	insigne::g_settings.post_draw_cmdbuff_arena_size_mb = 1;

	calyx::context_attribs* commonCtx = calyx::get_context_attribs();

	insigne::g_settings.native_res_x = commonCtx->window_width;
	insigne::g_settings.native_res_y = commonCtx->window_height;

	insigne::g_scene_settings.max_shaders = 32;
	insigne::g_scene_settings.max_ubos = 1024;
	insigne::g_scene_settings.max_ibos = 2048;
	insigne::g_scene_settings.max_vbos = 2048;
	insigne::g_scene_settings.max_textures = 256;
	insigne::g_scene_settings.max_fbos = 32;

	insigne::organize_memory();

	CLOVER_VERBOSE("Initialize filesystem");
	m_FSMemoryArena = g_PersistanceAllocator.allocate_arena<FreelistArena>(SIZE_MB(1));
	floral::absolute_path workingDir = floral::get_application_directory();
	m_FileSystem = floral::create_filesystem(workingDir, m_FSMemoryArena);
}

void Application::OnInitializeRenderer()
{
	LOG_TOPIC("app");
	CLOVER_VERBOSE("Initialize renderer settings");
	// graphics init
	insigne::initialize_driver();
	insigne::allocate_draw_command_buffers(5);
	insigne::allocate_post_draw_command_buffers(4);

	insigne::register_post_surface_type<DebugLine>();
	insigne::register_post_surface_type<DebugSurface>();
	insigne::register_post_surface_type<DebugTextSurface>();
	insigne::register_post_surface_type<ImGuiSurface>();

	insigne::initialize_render_thread();
	insigne::wait_for_initialization();

}

void Application::OnInitializeGame()
{
	LOG_TOPIC("app");
	CLOVER_VERBOSE("Initialize game settings");
	m_DemoHub = g_PersistanceAllocator.allocate<DemoHub>(m_FileSystem);
	m_DemoHub->Initialize();
	m_Initialized = true;
}

void Application::OnFrameStep(f32 i_deltaMs)
{
	if (m_Initialized)
	{
		UpdateFrame(i_deltaMs);

		insigne::begin_frame();
		RenderFrame(i_deltaMs);
		insigne::end_frame();
	}
}

void Application::OnCleanUp()
{
	m_DemoHub->CleanUp();
	insigne::clean_up_and_stop_render_thread();

	floral::destroy_filesystem(&m_FileSystem);
}

// -----------------------------------------
void Application::OnCharacterInput(c8 i_character)
{
	m_DemoHub->OnCharacterInput(i_character);
}

void Application::OnKeyInput(u32 i_keyCode, u32 i_keyStatus)
{
	m_DemoHub->OnKeyInput(i_keyCode, i_keyStatus);
}

void Application::OnCursorMove(u32 i_x, u32 i_y)
{
	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
	floral::vec2f nPos(f32(i_x) / commonCtx->window_width, -f32(i_y) / commonCtx->window_height);
	nPos = nPos * 2.0f + floral::vec2f(-1.0f, 1.0f);

	if (commonCtx->window_scale > 0.0f)
	{
		u32 x = u32((f32)i_x * commonCtx->window_scale);
		u32 y = u32((f32)i_y * commonCtx->window_scale);
		m_DemoHub->OnCursorMove(x, y);
	}
	else
	{
		m_DemoHub->OnCursorMove(i_x, i_y);
	}
}

void Application::OnCursorInteract(bool i_pressed, u32 i_buttonId)
{
	m_DemoHub->OnCursorInteract(i_pressed, i_buttonId);
}

}
