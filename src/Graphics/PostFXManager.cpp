#include "PostFXManager.h"

#include <context.h>
#include <insigne/render.h>

#include "Graphics/MaterialManager.h"
#include "Graphics/PostFXMaterial.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

// TODO: we can use a full screen triangle instead of a quad
static insigne::surface_handle_t				s_ScreenQuad;

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
			m_TonemapMaterial = (ToneMappingMaterial*)m_MaterialManager->CreateMaterial<ToneMappingMaterial>("shaders/internal/postfx/tonemap");
			m_GammaCorrectionMaterial = (GammaCorrectionMaterial*)m_MaterialManager->CreateMaterial<GammaCorrectionMaterial>("shaders/internal/postfx/gamma_correction");
			m_TonemapMaterial->SetExposure(0.7f);
		}

		{
			SSVertex vs[4];
			vs[0].Position = floral::vec2f(-1.0f, -1.0f);
			vs[0].TexCoord = floral::vec2f(0.0f, 0.0f);
			vs[1].Position = floral::vec2f(1.0f, -1.0f);
			vs[1].TexCoord = floral::vec2f(1.0f, 0.0f);
			vs[2].Position = floral::vec2f(1.0f, 1.0f);
			vs[2].TexCoord = floral::vec2f(1.0f, 1.0f);
			vs[3].Position = floral::vec2f(-1.0f, 1.0f);
			vs[3].TexCoord = floral::vec2f(0.0f, 1.0f);
			u32 indices[] = {0, 1, 2, 2, 3, 0};
			s_ScreenQuad = insigne::upload_surface(&vs[0], sizeof(SSVertex) * 4, &indices[0], sizeof(u32) * 6,
					sizeof(SSVertex), 4, 6);
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
			insigne::begin_render_pass(m_TonemapBuffer);
			m_TonemapMaterial->SetInputTex(mainColorTex);
			m_GammaCorrectionMaterial->SetInputTex(mainColorTex);
			insigne::draw_surface<SSSurface>(s_ScreenQuad, m_TonemapMaterial->GetHandle());
			insigne::end_render_pass(m_TonemapBuffer);
			insigne::dispatch_render_pass();
		}
	}

	void PostFXManager::RenderFinalPass()
	{
		// extract tone mapped scene
		insigne::texture_handle_t toneMappedTex = insigne::extract_color_attachment(m_TonemapBuffer, 0);
		// gamma correction
		// TODO: we can combine this pass with ToneMapping
		{
			PROFILE_SCOPE(GammaCorrection);
			m_GammaCorrectionMaterial->SetInputTex(toneMappedTex);
			insigne::draw_surface<SSSurface>(s_ScreenQuad, m_GammaCorrectionMaterial->GetHandle());
		}
	}

}
