#include "VisualComponent.h"

#include <insigne/render.h>
#include <lotus/profiler.h>

#include "Graphics/Camera.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/PBRMaterial.h"

namespace stone {
	VisualComponent::VisualComponent()
		: m_PositionWS(0.0f, 0.0f, 0.0f)
		, m_RotationWS(0.0f, 0.0f, 0.0f)
		, m_ScalingWS(1.0f, 1.0f, 1.0f)
	{
	}

	VisualComponent::~VisualComponent()
	{
	}

	void VisualComponent::Update(Camera* i_camera, f32 i_deltaMs)
	{
		PROFILE_SCOPE(UpdateVisualComponent);
		// transform update
		m_Transform = floral::construct_translation3d(m_PositionWS) * floral::construct_scaling3d(m_ScalingWS);
	}

	void VisualComponent::Render(Camera* i_camera)
	{
		PBRMaterial* mat = reinterpret_cast<PBRMaterial*>(m_Material);
		mat->SetWVP(i_camera->WVPMatrix);
		mat->SetTransform(m_Transform);
		insigne::draw_surface<SolidSurface>(m_Surface, m_Material->GetHandle());
	}

	void VisualComponent::Initialize(insigne::surface_handle_t i_surfaceHdl, IMaterial* i_matHdl)
	{
		m_Surface = i_surfaceHdl;
		m_Material = i_matHdl;
	}
}
