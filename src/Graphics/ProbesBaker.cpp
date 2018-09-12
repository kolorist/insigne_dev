#include "ProbesBaker.h"

#include <insigne/render.h>

namespace stone {

ProbesBaker::ProbesBaker()
	: m_IsReady(false)
{
}

ProbesBaker::~ProbesBaker()
{
}

void ProbesBaker::Initialize()
{
	{
		insigne::color_attachment_list_t* colorAttachs = insigne::allocate_color_attachment_list(1u);
		colorAttachs->push_back(
				insigne::color_attachment_t("mega_fbo",
					insigne::texture_format_e::hdr_rgba));
		insigne::framebuffer_handle_t fb = insigne::create_framebuffer(
				768, 768,
				1.0f,
				true,
				colorAttachs);
		m_ProbesFramebuffer = fb;
	}
	m_IsReady = true;
}

insigne::framebuffer_handle_t ProbesBaker::GetMegaFramebuffer()
{
	return m_ProbesFramebuffer;
}

bool ProbesBaker::IsReady()
{
	return m_IsReady;
}

}
