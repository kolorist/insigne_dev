#include "SingleAccurateFormFactor.h"

#include <floral/gpds/camera.h>

#include <imgui.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_textures.h>
#include <insigne/ut_shading.h>

#include <clover/Logger.h>

#include "InsigneImGui.h"
#include "Graphics/DebugDrawer.h"

namespace stone
{
namespace tools
{

static const_cstr k_SuiteName = "FormFactor - 1 Patches Pair";

SingleAccurateFormFactor::SingleAccurateFormFactor()
	: m_CameraMotion(
		floral::camera_view_t { floral::vec3f(6.0f, 6.0f, 6.0f), floral::vec3f(-3.0f, -3.0f, -3.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
		floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
{
}

SingleAccurateFormFactor::~SingleAccurateFormFactor()
{
}

const_cstr SingleAccurateFormFactor::GetName() const
{
	return k_SuiteName;
}

void SingleAccurateFormFactor::OnInitialize(floral::filesystem<FreelistArena>* i_fs)
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_SuiteName);
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	m_ParaConfig.UseStratifiedSample = false;
	m_ParaConfig.SamplesCount = 16;
	m_ParaConfig.Distance = 1.0f;

	m_PerpConfig.UseStratifiedSample = false;
	m_PerpConfig.SamplesCount = 16;
	m_PerpConfig.Angle = 90.0f;

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_SceneData.WVP = m_CameraMotion.GetWVP();

		insigne::update_ub(newUB, &m_SceneData, sizeof(SceneData), 0);
		m_UB = newUB;
	}
}

void SingleAccurateFormFactor::OnUpdate(const f32 i_deltaMs)
{
	// Debug UI update
	ImGui::Begin("Controller");

	ImGui::BeginChild("Controller", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f,0), false);
	ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Controller");
	ImGui::Separator();

	// parallel
	{
		ImGui::Text(">> Parallel Patches");
		ImGui::Checkbox("Use Stratified##para", &m_ParaConfig.UseStratifiedSample);
		ImGui::SameLine();
		if (ImGui::Button("Reset##para"))
		{
			m_ParaConfig.UseStratifiedSample = false;
			m_ParaConfig.SamplesCount = 16;
			m_ParaConfig.Distance = 1.0f;
		}
		ImGui::SliderInt("Samples##para", &m_ParaConfig.SamplesCount, 1, 32);
		ImGui::SliderFloat("Distance##para", &m_ParaConfig.Distance, 0.0f, 3.0f);
		ImGui::Button("Compute##para");
	}

	ImGui::NewLine();

	// perpendicular
	{
		ImGui::Text(">> Perpendicular Patches");
		ImGui::Checkbox("Use Stratified##perp", &m_PerpConfig.UseStratifiedSample);
		ImGui::SameLine();
		if (ImGui::Button("Reset##perp"))
		{
			m_PerpConfig.UseStratifiedSample = false;
			m_PerpConfig.SamplesCount = 16;
			m_PerpConfig.Angle = 90.0f;
		}
		ImGui::SliderInt("Samples##perp", &m_PerpConfig.SamplesCount, 1, 32);
		ImGui::SliderFloat("Angle##perp", &m_PerpConfig.Angle, 0.0f, 180.0f);
		ImGui::Button("Compute##perp");
	}

	ImGui::EndChild();

	////////////////////////////////////////////
	ImGui::SameLine();
	////////////////////////////////////////////

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("Value", ImVec2(0, 0), true);

	ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Computed Values");
	ImGui::Separator();
	{
		ImGui::Text(">> Parallel Patches");
		ImGui::Text("point2patch:");
		ImGui::Text("patch2patch:");
	}

	ImGui::NewLine();

	{
		ImGui::Text(">> Perpendicular Patches");
		ImGui::Text("point2patch:");
		ImGui::Text("patch2patch:");
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();

	ImGui::End();

	// Logic update
	m_CameraMotion.OnUpdate(i_deltaMs);
	m_SceneData.WVP = m_CameraMotion.GetWVP();

	_UpdateParallelVisual();
	_UpdatePerpendicularVisual();
}

void SingleAccurateFormFactor::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	debugdraw::Render(m_SceneData.WVP);
	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void SingleAccurateFormFactor::OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_SuiteName);

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

//----------------------------------------------

void SingleAccurateFormFactor::_UpdateParallelVisual()
{
	floral::vec3f tsl(-1.5f, 0.0f, 1.5f);
	floral::vec3f q0[] = {
		floral::vec3f(0.0f, 0.0f, 0.0f) + tsl,
		floral::vec3f(1.0f, 0.0f, 0.0f) + tsl,
		floral::vec3f(1.0f, 0.0f, 1.0f) + tsl,
		floral::vec3f(0.0f, 0.0f, 1.0f) + tsl
	};
	floral::vec3f q1[4] {
		q0[0] + floral::vec3f(0.0f, m_ParaConfig.Distance, 0.0f),
		q0[1] + floral::vec3f(0.0f, m_ParaConfig.Distance, 0.0f),
		q0[2] + floral::vec3f(0.0f, m_ParaConfig.Distance, 0.0f),
		q0[3] + floral::vec3f(0.0f, m_ParaConfig.Distance, 0.0f),
	};

	debugdraw::DrawQuad3D(q0[0], q0[1], q0[2], q0[3], floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	debugdraw::DrawQuad3D(q1[0], q1[1], q1[2], q1[3], floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
}

void SingleAccurateFormFactor::_UpdatePerpendicularVisual()
{
	floral::vec3f q0[] = {
		floral::vec3f(0.0f, 0.0f, 0.0f),
		floral::vec3f(1.0f, 0.0f, 0.0f),
		floral::vec3f(1.0f, 0.0f, 1.0f),
		floral::vec3f(0.0f, 0.0f, 1.0f)
	};
	floral::vec3f q1[4] {
		q0[0],
		q0[1],
		floral::vec3f(1.0f, sinf(floral::to_radians(m_PerpConfig.Angle)), cosf(floral::to_radians(m_PerpConfig.Angle))),
		floral::vec3f(0.0f, sinf(floral::to_radians(m_PerpConfig.Angle)), cosf(floral::to_radians(m_PerpConfig.Angle)))
	};

	debugdraw::DrawQuad3D(q0[0], q0[1], q0[2], q0[3], floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	debugdraw::DrawQuad3D(q1[0], q1[1], q1[2], q1[3], floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
}

}
}
