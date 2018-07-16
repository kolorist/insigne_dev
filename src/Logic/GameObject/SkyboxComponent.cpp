#include "SkyboxComponent.h"

#include <insigne/render.h>
#include <lotus/profiler.h>

#include "Graphics/SurfaceDefinitions.h"

namespace stone {
	SkyboxComponent::SkyboxComponent()
	{
	}

	SkyboxComponent::~SkyboxComponent()
	{
	}

	void SkyboxComponent::Update(f32 i_deltaMs)
	{
	}

	void SkyboxComponent::Render()
	{
		insigne::draw_surface<SkyboxSurface>(m_Surface, m_Material->GetHandle());
	}

	void SkyboxComponent::Initialize(insigne::surface_handle_t i_surfaceHdl, IMaterial* i_matHdl)
	{
		m_Surface = i_surfaceHdl;
		m_Material = i_matHdl;
	}
}
