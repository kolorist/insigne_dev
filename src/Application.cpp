#include "Application.h"

#include <floral.h>
#include <clover.h>
#include <lotus/events.h>
#include <lotus/profiler.h>
#include <insigne/configs.h>
#include <insigne/system.h>
#include <insigne/driver.h>
#include <insigne/ut_render.h>

#include "System/Controller.h"

#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/ICameraMotion.h"
#include "Graphics/IDebugUI.h"

#include "Graphics/Tests/ITestSuite.h"
#include "Graphics/Tests/PlainQuad.h"
#include "Graphics/Tests/SHBaking.h"
#include "Graphics/Tests/FormFactorsValidating.h"
#include "Graphics/Tests/CornelBox.h"
#include "Graphics/Tests/OctreePartition.h"
#include "Graphics/Tests/LightProbePlacement.h"
#include "Graphics/Tests/CbFormats.h"
#include "Graphics/Tests/DebugUITest.h"
#if 0
#include "Graphics/Tests/FormFactorsBaking.h"
#include "Graphics/Tests/PlainTextureQuad.h"
#include "Graphics/Tests/CubeMapTexture.h"
#include "Graphics/Tests/VectorMath.h"
#include "Graphics/Tests/GPUVectorMath.h"
#include "Graphics/Tests/OmniShadow.h"
#include "Graphics/Tests/SHMath.h"
#include "Graphics/Tests/GlobalIllumination.h"
#endif

namespace stone {

Application::Application(Controller* i_controller)
	: m_Initialized(false)
	, m_CurrentTestSuite(nullptr)
	, m_CurrentTestSuiteUI(nullptr)
{
	//s_profileEvents[0].init(4096u, &g_SystemAllocator);
	//s_profileEvents[1].init(4096u, &g_SystemAllocator);

	i_controller->IOEvents.OnInitialize.bind<Application, &Application::OnInitialize>(this);
	i_controller->IOEvents.OnPause.bind<Application, &Application::OnPause>(this);
	i_controller->IOEvents.OnResume.bind<Application, &Application::OnResume>(this);
	i_controller->IOEvents.OnFrameStep.bind<Application, &Application::OnFrameStep>(this);
	/*
	i_controller->IOEvents.OnFocusChanged.bind<Application, &Application::OnFocusChanged>(this);
	i_controller->IOEvents.OnDisplayChanged.bind<Application, &Application::OnDisplayChanged>(this);

	i_controller->IOEvents.OnCleanUp.bind<Application, &Application::OnCleanUp>(this);

	*/
	//i_controller->IOEvents.CharacterInput.bind<Application, &Application::OnCharacterInput>(this);
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
	PROFILE_SCOPE(UpdateFrame);
	if (m_CurrentTestSuite)
	{
		m_CurrentTestSuite->OnUpdate(i_deltaMs);

		m_CurrentTestSuiteUI->OnFrameUpdate(i_deltaMs);
	}
	/*
	s_profileEvents[0].empty();
	lotus::unpack_capture(s_profileEvents[0], 0);
	s_profileEvents[1].empty();
	lotus::unpack_capture(s_profileEvents[1], 1);
	*/
}

void Application::RenderFrame(f32 i_deltaMs)
{
	PROFILE_SCOPE(RenderFrame);
	if (m_CurrentTestSuite)
	{
		m_CurrentTestSuite->OnRender(i_deltaMs);

		m_CurrentTestSuiteUI->OnFrameRender(i_deltaMs);
	}
}

// -----------------------------------------
void Application::OnPause()
{
	// pause render thread
	insigne::pause_render_thread();
}

void Application::OnResume()
{
	// resume render thread
	insigne::resume_render_thread();
}

void Application::OnFocusChanged(bool i_hasFocus)
{
}

void Application::OnDisplayChanged()
{
	insigne::request_refresh_context();
}

void Application::OnInitialize()
{
	CLOVER_VERBOSE(__FUNCTION__);
	if (m_Initialized)
	{
		return;
	}

	// insigne settings
	insigne::g_settings.frame_shader_allocator_size_mb = 4u;
	insigne::g_settings.frame_buffers_allocator_size_mb = 48u;
	insigne::g_settings.frame_textures_allocator_size_mb = 48u;
	insigne::g_settings.frame_render_allocator_size_mb = 4u;
	insigne::g_settings.frame_draw_allocator_size_mb = 4u;

	//insigne::g_settings.draw_command_buffer_size = 2048u;

	insigne::g_settings.native_res_x = 1280u;
	insigne::g_settings.native_res_y = 720u;
	
	// graphics init
	insigne::initialize_driver();
	insigne::allocate_draw_command_buffers(6);

	insigne::register_surface_type<SurfacePNC>();
	//insigne::register_surface_type<SurfacePNC>();
	//insigne::register_surface_type<SurfacePNCC>();
	//insigne::register_surface_type<SurfaceP>();
	insigne::register_surface_type<DebugLine>();
	insigne::register_surface_type<ImGuiSurface>();

	insigne::initialize_render_thread();
	insigne::wait_for_initialization();

	//_CreateTestSuite<DebugUITest>();
	//_CreateTestSuite<CornelBox>();
	//_CreateTestSuite<OctreePartition>();
	_CreateTestSuite<LightProbePlacement>();

	if (m_CurrentTestSuite)
	{
		m_CurrentTestSuite->OnInitialize();
		m_CurrentTestSuiteUI->Initialize();
	}

	m_Initialized = true;
}

void Application::OnFrameStep(f32 i_deltaMs)
{
	if (m_Initialized)
	{
		insigne::begin_frame();
		UpdateFrame(i_deltaMs);
		RenderFrame(i_deltaMs);
		insigne::end_frame();
	}
}

void Application::OnCleanUp(int i_param)
{
	if (m_CurrentTestSuite)
		m_CurrentTestSuite->OnCleanUp();
}

// -----------------------------------------
void Application::OnCharacterInput(c8 i_character)
{
}

void Application::OnKeyInput(u32 i_keyCode, u32 i_keyStatus)
{
	if (i_keyStatus == 0) { // pressed
	} else if (i_keyStatus == 2) { // up
	}

	if (m_CurrentTestSuite->GetCameraMotion())
	{
		m_CurrentTestSuite->GetCameraMotion()->OnKeyInput(i_keyCode, i_keyStatus);
	}
	m_CurrentTestSuiteUI->OnKeyInput(i_keyCode, i_keyStatus);
}

void Application::OnCursorMove(u32 i_x, u32 i_y)
{
	if (m_CurrentTestSuite->GetCameraMotion())
	{
		m_CurrentTestSuite->GetCameraMotion()->OnCursorMove(i_x, i_y);
	}
	m_CurrentTestSuiteUI->OnCursorMove(i_x, i_y);
}

void Application::OnCursorInteract(bool i_pressed, u32 i_buttonId)
{
	if (m_CurrentTestSuite->GetCameraMotion())
	{
		m_CurrentTestSuite->GetCameraMotion()->OnCursorInteract(i_pressed);
	}
	m_CurrentTestSuiteUI->OnCursorInteract(i_pressed);
}

}
