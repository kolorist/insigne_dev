#include "SkyboxComponent.h"

#include <insigne/render.h>
#include <lotus/profiler.h>

#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/SkyboxMaterial.h"

namespace stone {
	SkyboxComponent::SkyboxComponent()
	{
	}

	SkyboxComponent::~SkyboxComponent()
	{
	}

	void SkyboxComponent::Update(f32 i_deltaMs)
	{
		m_Transform = floral::construct_translation3d(3.0f, 3.0f, 3.0f);
	}

	void SkyboxComponent::Render()
	{
		SkyboxMaterial* mat = (SkyboxMaterial*)m_Material;
		floral::mat4x4f v = floral::construct_lookat(
				floral::vec3f(0.0f, 1.0f, 0.0f),
				floral::vec3f(3.0f, 3.0f, 3.0f),
				floral::vec3f(-3.0f, -3.0f, -3.0f));
		floral::mat4x4f p = floral::construct_perspective(0.01f, 10.0f, 35.0f, 16.0f / 9.0f);
		floral::mat4x4f wvp = p * v;
		mat->SetWVP(wvp);
		mat->SetTransform(m_Transform);
		insigne::draw_surface<SkyboxSurface>(m_Surface, m_Material->GetHandle());
	}

	void SkyboxComponent::Initialize(insigne::surface_handle_t i_surfaceHdl, IMaterial* i_matHdl)
	{
		m_Surface = i_surfaceHdl;
		m_Material = i_matHdl;
	}
}
