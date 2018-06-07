#include "PostFXManager.h"

#include <context.h>
#include <insigne/render.h>

namespace stone {
	PostFXManager::PostFXManager()
	{
	}

	PostFXManager::~PostFXManager()
	{
	}

	void PostFXManager::Initialize()
	{
		insigne::color_attachment_list_t* colorAttachs = insigne::allocate_color_attachment_list(1u);
		colorAttachs->push_back(
				insigne::color_attachment_t("main_color",
					insigne::texture_format_e::hdr_rgba));
		insigne::framebuffer_handle_t fb = insigne::create_framebuffer(
				calyx::g_context_attribs->window_width,
				calyx::g_context_attribs->window_height,
				1.0f,
				true,
				colorAttachs);
		m_MainFramebuffer = fb;
	}
}
