#include "VisualComponent.h"

#include <insigne/render.h>

#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/PBRMaterial.h"

namespace stone {
	VisualComponent::VisualComponent()
	{
	}

	VisualComponent::~VisualComponent()
	{
	}

	void VisualComponent::Update(f32 i_deltaMs)
	{
		// nothing
	}

	void VisualComponent::Render()
	{
		PBRMaterial* mat = reinterpret_cast<PBRMaterial*>(m_Material);
		mat->SetWVP(floral::mat4x4f(1.0f));
		mat->SetTransform(floral::mat4x4f(1.0f));
		insigne::draw_surface<SolidSurface>(m_Surface, m_Material->GetHandle());
	}

	void VisualComponent::Initialize(insigne::surface_handle_t i_surfaceHdl, IMaterial* i_matHdl)
	{
		m_Surface = i_surfaceHdl;
		m_Material = i_matHdl;
	}
}
