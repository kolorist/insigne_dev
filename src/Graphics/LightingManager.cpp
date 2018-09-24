#include "LightingManager.h"

#include <insigne/render.h>

namespace stone {

LightingManager::LightingManager(Game* i_game)
	: m_Game(i_game)
{
}

LightingManager::~LightingManager()
{
}

void LightingManager::Initialize()
{
	{
		insigne::color_attachment_list_t* colorAttachs = insigne::allocate_color_attachment_list(1u);
		colorAttachs->push_back(
				insigne::color_attachment_t("cube_depth",
					insigne::texture_format_e::hdr_rgba));
		insigne::framebuffer_handle_t fb = insigne::create_framebuffer(
				768, 128,
				1.0f,
				true,
				colorAttachs);
	}
}

void LightingManager::RenderShadowMap()
{
}

insigne::texture_handle_t LightingManager::GetShadowMap()
{
	return insigne::texture_handle_t();
}

}
