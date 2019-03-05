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
	IDebugUI();

	void										Initialize();

	virtual void								OnDebugUIUpdate(const f32 i_deltaMs) = 0;

	void										OnFrameUpdate(const f32 i_deltaMs);
	void										OnFrameRender(const f32 i_deltaMs);

	void										OnKeyInput(const u32 i_keyCode, const u32 i_keyStatus);
	void										OnCursorMove(const u32 i_x, const u32 i_y);
	void										OnCursorInteract(const bool i_pressed);

private:
	void										RenderImGui(ImDrawData* i_drawData);

private:
	const s32									AllocateNewBuffers();

private:
	floral::inplace_array<insigne::vb_handle_t, 8>	m_VBs;
	floral::inplace_array<insigne::ib_handle_t, 8>	m_IBs;
	insigne::ub_handle_t						m_UB;

	insigne::shader_handle_t					m_Shader;
	insigne::texture_handle_t					m_Texture;
	insigne::material_desc_t					m_Material;

private:
	floral::vec2f								m_CursorPos;
	bool										m_CursorPressed;
	bool										m_CursorHeldThisFrame;

	bool										m_ShowDebugMenu;
	bool										m_ShowDebugInfo;
	bool										m_ShowInsigneInfo;
};

}
