#include "VisualComponent.h"

#include <insigne/render.h>

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
	}

	void VisualComponent::Initialize(insigne::surface_handle_t i_surfaceHdl, IMaterial* i_matHdl)
	{
		m_Surface = i_surfaceHdl;
		m_Material = i_matHdl;
	}
}
