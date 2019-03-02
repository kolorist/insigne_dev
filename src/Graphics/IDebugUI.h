#pragma once

#include <floral.h>
#include <imgui.h>
#include <insigne/commons.h>

#include "Graphics/SurfaceDefinitions.h"

namespace stone
{

class IDebugUI
{
public:
	IDebugUI() {}

	void										Initialize();

	virtual void								OnDebugUIUpdate(const f32 i_deltaMs) = 0;

	void										OnFrameUpdate(const f32 i_deltaMs);
	void										OnFrameRender(const f32 i_deltaMs);

private:
	static void									RenderImGuiDrawLists(ImDrawData* i_drawData);

private:
	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;

	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;
};

}
