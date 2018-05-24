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
		floral::mat4x4f v = floral::construct_lookat(
				floral::vec3f(0.0f, 1.0f, 0.0f),
				floral::vec3f(3.0f, 3.0f, 3.0f),
				floral::vec3f(-3.0f, -3.0f, -3.0f));
		floral::mat4x4f p = floral::construct_perspective(0.01f, 100.0f, 35.0f, 16.0f / 9.0f);
		floral::mat4x4f wvp = p * v;
		mat->SetWVP(wvp);
		mat->SetTransform(floral::construct_scaling3d(0.2f, 0.2f, 0.2f));
		insigne::draw_surface<SolidSurface>(m_Surface, m_Material->GetHandle());
	}

	void VisualComponent::Initialize(insigne::surface_handle_t i_surfaceHdl, IMaterial* i_matHdl)
	{
		m_Surface = i_surfaceHdl;
		m_Material = i_matHdl;
	}
}
