#include "Application.h"

#include <floral.h>
#include <clover.h>
#include <lotus/events.h>
#include <lotus/profiler.h>
#include <insigne/system.h>
#include <insigne/driver.h>
#include <insigne/ut_render.h>

#include "Memory/MemorySystem.h"
#include "System/Controller.h"

#include "Graphics/SurfaceDefinitions.h"

#include "Graphics/Tests/ITestSuite.h"
#include "Graphics/Tests/PlainQuad.h"
#include "Graphics/Tests/PlainTextureQuad.h"
#include "Graphics/Tests/CubeMapTexture.h"
#include "Graphics/Tests/VectorMath.h"
#include "Graphics/Tests/GPUVectorMath.h"
#include "Graphics/Tests/CornelBox.h"
#include "Graphics/Tests/OmniShadow.h"
#include "Graphics/Tests/GlobalIllumination.h"

namespace stone {

Application::Application(Controller* i_controller)
{
	//s_profileEvents[0].init(4096u, &g_SystemAllocator);
	//s_profileEvents[1].init(4096u, &g_SystemAllocator);

	i_controller->IOEvents.OnInitialize.bind<Application, &Application::OnInitialize>(this);
	i_controller->IOEvents.OnFrameStep.bind<Application, &Application::OnFrameStep>(this);
	i_controller->IOEvents.OnCleanUp.bind<Application, &Application::OnCleanUp>(this);

	i_controller->IOEvents.CharacterInput.bind<Application, &Application::OnCharacterInput>(this);
	i_controller->IOEvents.KeyInput.bind<Application, &Application::OnKeyInput>(this);
	i_controller->IOEvents.CursorMove.bind<Application, &Application::OnCursorMove>(this);
	i_controller->IOEvents.CursorInteract.bind<Application, &Application::OnCursorInteract>(this);

	//m_CurrentTestSuite = g_PersistanceAllocator.allocate<PlainQuadTest>();
	//m_CurrentTestSuite = g_PersistanceAllocator.allocate<PlainTextureQuad>();
	//m_CurrentTestSuite = g_PersistanceAllocator.allocate<CubeMapTexture>();
	//m_CurrentTestSuite = g_PersistanceAllocator.allocate<VectorMath>();
	//m_CurrentTestSuite = g_PersistanceAllocator.allocate<GPUVectorMath>();
	//m_CurrentTestSuite = g_PersistanceAllocator.allocate<CornelBox>();
	//m_CurrentTestSuite = g_PersistanceAllocator.allocate<OmniShadow>();
	m_CurrentTestSuite = g_PersistanceAllocator.allocate<GlobalIllumination>();
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
		m_CurrentTestSuite->OnUpdate(i_deltaMs);
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
		m_CurrentTestSuite->OnRender(i_deltaMs);
}

// -----------------------------------------
void Application::OnInitialize(int i_param)
{
	// insigne settings
	insigne::g_settings.frame_shader_allocator_size_mb = 4u;
	insigne::g_settings.frame_buffers_allocator_size_mb = 16u;
	insigne::g_settings.frame_textures_allocator_size_mb = 48u;
	insigne::g_settings.frame_render_allocator_size_mb = 4u;
	insigne::g_settings.frame_draw_allocator_size_mb = 4u;

	insigne::g_settings.draw_command_buffer_size = 64u;
	
	// graphics init
	insigne::initialize_driver();

	// the rendering order is
	// 1- SolidSurface
	// 2- Skybox
	// 3- ScreenSpaceSurface
	// 4- ImGuiSurface
	// thus the order of declaration must be reversed
	typedef type_list_4(DebugLine, SurfacePNC, DemoSurface, DemoTexturedSurface)		SurfaceTypeList;
	insigne::initialize_render_thread<SurfaceTypeList>();
	insigne::wait_for_initialization();

	if (m_CurrentTestSuite)
		m_CurrentTestSuite->OnInitialize();
}

void Application::OnFrameStep(f32 i_deltaMs)
{
	insigne::begin_frame();
	UpdateFrame(i_deltaMs);
	RenderFrame(i_deltaMs);
	insigne::end_frame();
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
}

void Application::OnCursorMove(u32 i_x, u32 i_y)
{
}

void Application::OnCursorInteract(bool i_pressed, u32 i_buttonId)
{
}
}
