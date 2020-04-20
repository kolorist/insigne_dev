#include "CameraWork.h"

#include <floral/containers/array.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/DebugDrawer.h"

namespace stone
{
namespace misc
{
//-------------------------------------------------------------------

static const floral::vec3f k_OriginUpDir(0.0f, 0.0f, 1.0f);
static const floral::vec3f k_OriginLookAtDir(1.0f, 0.0f, 0.0f);

CameraWork::CameraWork()
	: m_UseRightHanded(true)
	, m_UsePerspectiveProjection(true)
{
}

//-------------------------------------------------------------------

CameraWork::~CameraWork()
{
}

//-------------------------------------------------------------------

ICameraMotion* CameraWork::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr CameraWork::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void CameraWork::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);

	_ResetValues();
	_CalculateCamera();
}

//-------------------------------------------------------------------

void CameraWork::_OnUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("Controller");

	bool modified = false;
	if (ImGui::Button("reset"))
	{
		_ResetValues();
		modified |= true;
	}

	ImGui::DragFloat("grid spacing", &m_GridSpacing, 0.01f, 0.5f, 2.0f);
	ImGui::DragInt("grid range", &m_GridRange, 0.05f, 3, 20);

	if (ImGui::RadioButton("right handed", m_UseRightHanded == true))
	{
		m_UseRightHanded = true;
		modified |= true;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("left handed", m_UseRightHanded == false))
	{
		m_UseRightHanded = false;
		modified |= true;
	}

	modified |= DebugVec3f("camera position", &m_CameraPosition);
	modified |= DebugVec3f("look at", &m_LookAt);
	modified |= DebugVec3f("up direction", &m_UpDirection);

	modified |= ImGui::InputFloat("near", &m_Near);
	modified |= ImGui::InputFloat("far", &m_Far);
	if (ImGui::RadioButton("perspective", m_UsePerspectiveProjection == true))
	{
		m_UsePerspectiveProjection = true;
		modified |= true;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("orthographic", m_UsePerspectiveProjection == false))
	{
		m_UsePerspectiveProjection = false;
		modified |= true;
	}

	if (m_UsePerspectiveProjection)
	{
		modified |= ImGui::DragFloat("fovy", &m_FovY, 1.0f, 0.0f, 180.0f);
		modified |= ImGui::DragFloat("aspect ratio", &m_AspectRatio, 0.01f, 0.0f, 3.0f);
	}
	else
	{
		modified |= ImGui::InputFloat("left", &m_Left);
		modified |= ImGui::InputFloat("right", &m_Right);
		modified |= ImGui::InputFloat("top", &m_Top);
		modified |= ImGui::InputFloat("bottom", &m_Bottom);
	}

	if (modified)
	{
		_CalculateCamera();
	}

	ImGui::Text("calculated matrices");
	DebugMat4fRowOrder("view", &m_ViewMatrix);
	DebugMat4fRowOrder("projection", &m_ProjectionMatrix);

	ImGui::End();

	// coordinate
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

void CameraWork::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	debugdraw::Render(m_ViewProjectionMatrix);
	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

//-------------------------------------------------------------------

void CameraWork::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
}

//-------------------------------------------------------------------

void CameraWork::_CalculateCamera()
{
	if (m_UseRightHanded)
	{
		m_ViewMatrix = floral::construct_lookat_point_rh(m_UpDirection, m_CameraPosition, m_LookAt);

		if (m_UsePerspectiveProjection)
		{
			m_ProjectionMatrix = floral::construct_perspective_rh(m_Near, m_Far, m_FovY, m_AspectRatio);
		}
		else
		{
			m_ProjectionMatrix = floral::construct_orthographic_rh(m_Left, m_Right, m_Top, m_Bottom, m_Near, m_Far);
		}
	}
	else
	{
		m_ViewMatrix = floral::construct_lookat_point_lh(m_UpDirection, m_CameraPosition, m_LookAt);
		if (m_UsePerspectiveProjection)
		{
			m_ProjectionMatrix = floral::construct_perspective_lh(m_Near, m_Far, m_FovY, m_AspectRatio);
		}
		else
		{
			m_ProjectionMatrix = floral::construct_orthographic_lh(m_Left, m_Right, m_Top, m_Bottom, m_Near, m_Far);
		}
	}
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void CameraWork::_ResetValues()
{
	m_GridSpacing = 1.0f;
	m_GridRange = 5;

	m_LookAt = floral::vec3f(0.0f, 0.0f, 0.0f);
	m_UpDirection = k_OriginUpDir;
	m_CameraPosition = floral::vec3f(4.0f, 4.0f, 4.0f);

	m_Near = 0.01f;
	m_Far = 100.0f;

	m_FovY = 45.0f;
	m_AspectRatio = 16.0f / 9.0f;

	m_Left = -3.0f;
	m_Right = 3.0f;
	m_Top = 1.6875f;
	m_Bottom = -1.6875f;
}

//-------------------------------------------------------------------
}
}
