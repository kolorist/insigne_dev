#include "PostFXManager.h"

#include <context.h>
#include <insigne/render.h>

#include "Graphics/MaterialManager.h"
#include "Graphics/PostFXMaterial.h"

namespace stone {
	PostFXManager::PostFXManager(MaterialManager* i_materialManager)
		: m_MaterialManager(i_materialManager)
	{
	}

	PostFXManager::~PostFXManager()
	{
	}

	void PostFXManager::Initialize()
	{
		{
			m_TonemapMaterial = (ToneMappingMaterial*)m_MaterialManager->CreateMaterial<ToneMappingMaterial>("shaders/internal/tonemap");
			m_GammaCorrectionMaterial = (GammaCorrectionMaterial*)m_MaterialManager->CreateMaterial<GammaCorrectionMaterial>("shaders/internal/gamma_correction");
		}

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

		{
			insigne::color_attachment_list_t* colorAttachs = insigne::allocate_color_attachment_list(1u);
			colorAttachs->push_back(
					insigne::color_attachment_t("tone_mapped",
						insigne::texture_format_e::rgba));
			insigne::framebuffer_handle_t fb = insigne::create_framebuffer(
					calyx::g_context_attribs->window_width,
					calyx::g_context_attribs->window_height,
					1.0f,
					true,
					colorAttachs);
			m_TonemapBuffer = fb;
		}

		{
			insigne::color_attachment_list_t* colorAttachs = insigne::allocate_color_attachment_list(1u);
			colorAttachs->push_back(
					insigne::color_attachment_t("gamma_corrected",
						insigne::texture_format_e::rgba));
			insigne::framebuffer_handle_t fb = insigne::create_framebuffer(
					calyx::g_context_attribs->window_width,
					calyx::g_context_attribs->window_height,
					1.0f,
					true,
					colorAttachs);
			m_GammaCorrectionBuffer = fb;
		}
	}

	void PostFXManager::Render()
	{
		// extract main color texture
		insigne::texture_handle_t mainColorTex = insigne::extract_color_attachment(m_MainFramebuffer, 0);

		// tone mapping
		{
			PROFILE_SCOPE(ToneMapping);
			
		}
		// gamma correction
		// TODO: we can combine this pass with ToneMapping
		{
			PROFILE_SCOPE(GammaCorrection);
		}
	}
}
