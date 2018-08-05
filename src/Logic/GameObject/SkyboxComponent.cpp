#include "SkyboxComponent.h"

#include <insigne/render.h>
#include <lotus/profiler.h>

#include "Graphics/Camera.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/SkyboxMaterial.h"

namespace stone {
	SkyboxComponent::SkyboxComponent()
	{
	}

	SkyboxComponent::~SkyboxComponent()
	{
	}

	void SkyboxComponent::Update(Camera* i_camera, f32 i_deltaMs)
	{
		m_Transform = floral::construct_translation3d(i_camera->Position);
	}

	void SkyboxComponent::Render(Camera* i_camera)
	{
		SkyboxMaterial* mat = (SkyboxMaterial*)m_Material;
		mat->SetWVP(i_camera->WVPMatrix);
		mat->SetTransform(m_Transform);
		insigne::draw_surface<SkyboxSurface>(m_Surface, m_Material->GetHandle());
	}

	void SkyboxComponent::Initialize(insigne::surface_handle_t i_surfaceHdl, IMaterial* i_matHdl)
	{
		m_Surface = i_surfaceHdl;
		m_Material = i_matHdl;
	}
}
