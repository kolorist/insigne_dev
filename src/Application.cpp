#include "Application.h"

#include <floral.h>
#include <insigne/driver.h>
#include <insigne/render.h>

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/TextureManager.h"
#include "Graphics/ModelManager.h"

namespace stone {
	Application::Application(Controller* i_controller)
	{
		i_controller->IOEvents.OnInitialize.bind<Application, &Application::OnInitialize>(this);
		i_controller->IOEvents.OnFrameStep.bind<Application, &Application::OnFrameStep>(this);
		i_controller->IOEvents.OnCleanUp.bind<Application, &Application::OnCleanUp>(this);

		i_controller->IOEvents.CharacterInput.bind<Application, &Application::OnCharacterInput>(this);
		i_controller->IOEvents.CursorMove.bind<Application, &Application::OnCursorMove>(this);
		i_controller->IOEvents.CursorInteract.bind<Application, &Application::OnCursorInteract>(this);

		m_ShaderManager = g_SystemAllocator.allocate<ShaderManager>();
		m_MaterialManager = g_SystemAllocator.allocate<MaterialManager>(m_ShaderManager);
		m_TextureManager = g_SystemAllocator.allocate<TextureManager>();
		m_ModelManager = g_SystemAllocator.allocate<ModelManager>();
		m_Debugger = g_SystemAllocator.allocate<Debugger>(m_MaterialManager, m_TextureManager);
	}

	Application::~Application()
	{
		// TODO
	}

	// -----------------------------------------
	void Application::UpdateFrame(f32 i_deltaMs)
	{
		m_Debugger->Update(i_deltaMs);
	}

	void Application::RenderFrame(f32 i_deltaMs)
	{
		insigne::begin_frame();
		m_Debugger->Render(i_deltaMs);
		insigne::end_frame();
		insigne::dispatch_frame();
	}

	// -----------------------------------------
	void Application::OnInitialize(int i_param)
	{
		// graphics init
		insigne::initialize_driver();
		typedef type_list_1(ImGuiSurface)		SurfaceTypeList;
		insigne::initialize_render_thread<SurfaceTypeList>();
		insigne::wait_for_initialization();

		insigne::set_clear_color(0.3f, 0.4f, 0.5f, 1.0f);

		m_Debugger->Initialize();
		m_ShaderManager->Initialize();
		m_ModelManager->Initialize();

		m_ModelManager->CreateSingleSurface("gfx/envi/models/demo/cube.cbobj");
		m_ModelManager->CreateSingleSurface("gfx/go/models/demo/uv_sphere_pbr.cbobj");
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

	void Application::OnCursorMove(u32 i_x, u32 i_y)
	{
		m_Debugger->OnCursorMove(i_x, i_y);
	}

	void Application::OnCursorInteract(bool i_pressed, u32 i_buttonId)
	{
		m_Debugger->OnCursorInteract(i_pressed, i_buttonId);
	}
}
