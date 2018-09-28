#include "Application.h"

#include <floral.h>
#include <clover.h>
#include <lotus/events.h>
#include <lotus/profiler.h>
#include <insigne/driver.h>
#include <insigne/render.h>

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/TextureManager.h"
#include "Graphics/ModelManager.h"
#include "Graphics/PostFXManager.h"
#include "Graphics/ProbesBaker.h"
#include "Graphics/LightingManager.h"
#include "Graphics/DebugRenderer.h"

#include "Graphics/FBODebugMaterial.h"

namespace stone {

static insigne::surface_handle_t			s_testSS;
static FBODebugMaterial*					s_mat;

Application::Application(Controller* i_controller)
{
	s_profileEvents[0].init(4096u, &g_SystemAllocator);
	s_profileEvents[1].init(4096u, &g_SystemAllocator);

	i_controller->IOEvents.OnInitialize.bind<Application, &Application::OnInitialize>(this);
	i_controller->IOEvents.OnFrameStep.bind<Application, &Application::OnFrameStep>(this);
	i_controller->IOEvents.OnCleanUp.bind<Application, &Application::OnCleanUp>(this);

	i_controller->IOEvents.CharacterInput.bind<Application, &Application::OnCharacterInput>(this);
	i_controller->IOEvents.KeyInput.bind<Application, &Application::OnKeyInput>(this);
	i_controller->IOEvents.CursorMove.bind<Application, &Application::OnCursorMove>(this);
	i_controller->IOEvents.CursorInteract.bind<Application, &Application::OnCursorInteract>(this);

	m_ShaderManager = g_SystemAllocator.allocate<ShaderManager>();
	m_TextureManager = g_SystemAllocator.allocate<TextureManager>();
	m_MaterialManager = g_SystemAllocator.allocate<MaterialManager>(m_ShaderManager, m_TextureManager);
	m_ModelManager = g_SystemAllocator.allocate<ModelManager>(m_MaterialManager);
	m_PostFXManager = g_SystemAllocator.allocate<PostFXManager>(m_MaterialManager);
	m_Debugger = g_SystemAllocator.allocate<Debugger>(m_MaterialManager, m_TextureManager);
	m_Game = g_SystemAllocator.allocate<Game>(m_ModelManager, m_MaterialManager, m_TextureManager,
			m_Debugger);
	m_ProbesBaker = g_SystemAllocator.allocate<ProbesBaker>(m_Game, m_ModelManager);
	m_LightingManager = g_SystemAllocator.allocate<LightingManager>(m_Game, m_MaterialManager);
	m_DebugRenderer = g_SystemAllocator.allocate<DebugRenderer>(m_MaterialManager);
}

Application::~Application()
{
	// TODO
}

// -----------------------------------------
void Application::UpdateFrame(f32 i_deltaMs)
{
	PROFILE_SCOPE(UpdateFrame);
	{
		PROFILE_SCOPE(GameUpdate);
		m_Game->Update(i_deltaMs);
	}
	{
		PROFILE_SCOPE(DebuggerUpdate);
		m_Debugger->Update(i_deltaMs);
	}

	s_profileEvents[0].empty();
	lotus::unpack_capture(s_profileEvents[0], 0);
	s_profileEvents[1].empty();
	lotus::unpack_capture(s_profileEvents[1], 1);
}

void Application::RenderFrame(f32 i_deltaMs)
{
	PROFILE_SCOPE(RenderFrame);
	insigne::framebuffer_handle_t mainFb = m_PostFXManager->GetMainFramebuffer();
	insigne::texture_handle_t tex0 = insigne::extract_color_attachment(mainFb, 0);

	insigne::begin_frame();
	
	// main color buffer population
	{
		PROFILE_SCOPE(MainColorBufferRender);
		insigne::begin_render_pass(mainFb);
		m_Game->Render();
		m_DebugRenderer->Render(m_Game->GetMainCamera());
		m_ProbesBaker->RenderProbes(m_Game->GetMainCamera());
		insigne::end_render_pass(mainFb);
		insigne::dispatch_render_pass();
	}

#if 0
	{
		PROFILE_SCOPE(ProbeBaking);
		m_ProbesBaker->Render();
	}
#endif
	{
		m_LightingManager->RenderShadowMap();
	}

	// postfx
	{
		PROFILE_SCOPE(PostFX);
		m_PostFXManager->Render();
	}

	// final pass
	insigne::begin_render_pass(-1);
	{
		PROFILE_SCOPE(PFX_GammaCorrection);
		m_PostFXManager->RenderFinalPass();
		//s_mat->SetColorTex0(tex0);
		//insigne::draw_surface<SSSurface>(s_testSS, s_mat->GetHandle());
	}
	m_Debugger->Render(i_deltaMs);
	insigne::end_render_pass(-1);
	
	{
		PROFILE_SCOPE(PresentRender);
		insigne::mark_present_render();
		insigne::dispatch_render_pass();
	}

	insigne::end_frame();
}

// -----------------------------------------
void Application::OnInitialize(int i_param)
{
	// insigne settings
	insigne::g_renderer_settings.frame_allocator_size_mb = 64u;
	insigne::g_renderer_settings.draw_command_buffer_size = 64u;
	insigne::g_renderer_settings.generic_command_buffer_size = 128u;
	insigne::g_renderer_settings.frame_shader_allocator_size_mb = 16u;
	
	// graphics init
	insigne::initialize_driver();

	// the rendering order is
	// 1- SolidSurface
	// 2- Skybox
	// 3- ScreenSpaceSurface
	// 4- ImGuiSurface
	// thus the order of declaration must be reversed
	typedef type_list_4(ImGuiSurface, SSSurface, SolidSurface, SkyboxSurface)		SurfaceTypeList;
	insigne::initialize_render_thread<SurfaceTypeList>();
	insigne::wait_for_initialization();

	insigne::set_clear_color(0.0f, 0.0f, 0.0f, 0.0f);

	m_Debugger->Initialize();
	m_TextureManager->Initialize(32);
	m_ShaderManager->Initialize();
	m_ModelManager->Initialize();
	m_PostFXManager->Initialize();
	m_Game->Initialize();
	m_ProbesBaker->Initialize(floral::aabb3f(
				floral::vec3f(-0.5f, -0.3f, -0.5f),
				floral::vec3f(0.5f, 0.7f, 0.5f)));
	m_LightingManager->Initialize();
	m_DebugRenderer->Initialize();

	s_mat = (FBODebugMaterial*)m_MaterialManager->CreateMaterial<FBODebugMaterial>("shaders/internal/ssquad");
	insigne::shader_handle_t demoShader = m_ShaderManager->LoadShader2(floral::path("gfx/shd/shdebug.shd"));
	SSVertex vs[4];
	vs[0].Position = floral::vec2f(-1.0f, -1.0f);
	vs[0].TexCoord = floral::vec2f(0.0f, 0.0f);
	vs[1].Position = floral::vec2f(1.0f, -1.0f);
	vs[1].TexCoord = floral::vec2f(1.0f, 0.0f);
	vs[2].Position = floral::vec2f(1.0f, 1.0f);
	vs[2].TexCoord = floral::vec2f(1.0f, 1.0f);
	vs[3].Position = floral::vec2f(-1.0f, 1.0f);
	vs[3].TexCoord = floral::vec2f(0.0f, 1.0f);
	u32 indices[] = {0, 1, 2, 2, 3, 0};
	s_testSS = insigne::upload_surface(&vs[0], sizeof(SSVertex) * 4, &indices[0], sizeof(u32) * 6,
			sizeof(SSVertex), 4, 6);
}

void Application::OnFrameStep(f32 i_deltaMs)
{
	UpdateFrame(i_deltaMs);
	RenderFrame(i_deltaMs);
}

void Application::OnCleanUp(int i_param)
{
}

// -----------------------------------------
void Application::OnCharacterInput(c8 i_character)
{
	m_Debugger->OnCharacterInput(i_character);
}

void Application::OnKeyInput(u32 i_keyCode, u32 i_keyStatus)
{
	if (i_keyStatus == 0) { // pressed
		m_Game->OnKey(i_keyCode, true);
	} else if (i_keyStatus == 2) { // up
		m_Game->OnKey(i_keyCode, false);
	}
}

void Application::OnCursorMove(u32 i_x, u32 i_y)
{
	m_Debugger->OnCursorMove(i_x, i_y);
}

void Application::OnCursorInteract(bool i_pressed, u32 i_buttonId)
{
	m_Debugger->OnCursorInteract(i_pressed, i_buttonId);
}
}
