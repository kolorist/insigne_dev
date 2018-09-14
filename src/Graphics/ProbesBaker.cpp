#include "ProbesBaker.h"

#include <insigne/render.h>

#include "Logic/Game.h"

namespace stone {

ProbesBaker::ProbesBaker(Game* i_game)
	: m_IsReady(false)
	, m_Game(i_game)
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

	m_MemoryArena = g_PersistanceAllocator.allocate_arena<LinearArena>(SIZE_MB(2));

	m_ProbeCameras.init(2, m_MemoryArena);
	{
		ProbeCamera pCam;
		pCam.posXCam.Position = floral::vec3f(0.0f, 0.0f, 0.0f);
		pCam.posXCam.LookAtDir = floral::vec3f(0.0f, 1.0f, 0.0f);
		pCam.posXCam.AspectRatio = 1.0f;
		pCam.posXCam.NearPlane = 0.01f;
		pCam.posXCam.FarPlane = 1000.0f;
		pCam.posXCam.FOV = 90.0f;
		pCam.posXCam.ViewMatrix = floral::construct_lookat(
				floral::vec3f(0.0f, 0.0f, 1.0f),
				pCam.posXCam.Position,
				pCam.posXCam.LookAtDir);
		pCam.posXCam.ProjectionMatrix = floral::construct_perspective(
				pCam.posXCam.NearPlane,
				pCam.posXCam.FarPlane,
				pCam.posXCam.FOV,
				pCam.posXCam.AspectRatio);
		pCam.posXCam.WVPMatrix = pCam.posXCam.ProjectionMatrix * pCam.posXCam.ViewMatrix;

		pCam.posYCam.Position = floral::vec3f(0.0f, 0.0f, 0.0f);
		pCam.posYCam.LookAtDir = floral::vec3f(1.0f, 0.0f, 0.0f);
		pCam.posYCam.AspectRatio = 1.0f;
		pCam.posYCam.NearPlane = 0.01f;
		pCam.posYCam.FarPlane = 1000.0f;
		pCam.posYCam.FOV = 90.0f;
		pCam.posYCam.ViewMatrix = floral::construct_lookat(
				floral::vec3f(0.0f, 1.0f, 0.0f),
				pCam.posYCam.Position,
				pCam.posYCam.LookAtDir);
		pCam.posYCam.ProjectionMatrix = floral::construct_perspective(
				pCam.posYCam.NearPlane,
				pCam.posYCam.FarPlane,
				pCam.posYCam.FOV,
				pCam.posYCam.AspectRatio);
		pCam.posYCam.WVPMatrix = pCam.posYCam.ProjectionMatrix * pCam.posYCam.ViewMatrix;

		pCam.posZCam.Position = floral::vec3f(0.0f, 0.0f, 0.0f);
		pCam.posZCam.LookAtDir = floral::vec3f(0.0f, 0.0f, 1.0f);
		pCam.posZCam.AspectRatio = 1.0f;
		pCam.posZCam.NearPlane = 0.01f;
		pCam.posZCam.FarPlane = 1000.0f;
		pCam.posZCam.FOV = 90.0f;
		pCam.posZCam.ViewMatrix = floral::construct_lookat(
				floral::vec3f(0.0f, 1.0f, 0.0f),
				pCam.posZCam.Position,
				pCam.posZCam.LookAtDir);
		pCam.posZCam.ProjectionMatrix = floral::construct_perspective(
				pCam.posZCam.NearPlane,
				pCam.posZCam.FarPlane,
				pCam.posZCam.FOV,
				pCam.posZCam.AspectRatio);
		pCam.posZCam.WVPMatrix = pCam.posZCam.ProjectionMatrix * pCam.posZCam.ViewMatrix;

		pCam.negXCam.Position = floral::vec3f(0.0f, 0.0f, 0.0f);
		pCam.negXCam.LookAtDir = floral::vec3f(0.0f, -1.0f, 0.0f);
		pCam.negXCam.AspectRatio = 1.0f;
		pCam.negXCam.NearPlane = 0.01f;
		pCam.negXCam.FarPlane = 1000.0f;
		pCam.negXCam.FOV = 90.0f;
		pCam.negXCam.ViewMatrix = floral::construct_lookat(
				floral::vec3f(0.0f, 0.0f, -1.0f),
				pCam.negXCam.Position,
				pCam.negXCam.LookAtDir);
		pCam.negXCam.ProjectionMatrix = floral::construct_perspective(
				pCam.negXCam.NearPlane,
				pCam.negXCam.FarPlane,
				pCam.negXCam.FOV,
				pCam.negXCam.AspectRatio);
		pCam.negXCam.WVPMatrix = pCam.negXCam.ProjectionMatrix * pCam.negXCam.ViewMatrix;

		pCam.negYCam.Position = floral::vec3f(0.0f, 0.0f, 0.0f);
		pCam.negYCam.LookAtDir = floral::vec3f(-1.0f, 0.0f, 0.0f);
		pCam.negYCam.AspectRatio = 1.0f;
		pCam.negYCam.NearPlane = 0.01f;
		pCam.negYCam.FarPlane = 1000.0f;
		pCam.negYCam.FOV = 90.0f;
		pCam.negYCam.ViewMatrix = floral::construct_lookat(
				floral::vec3f(0.0f, 1.0f, 0.0f),
				pCam.negYCam.Position,
				pCam.negYCam.LookAtDir);
		pCam.negYCam.ProjectionMatrix = floral::construct_perspective(
				pCam.negYCam.NearPlane,
				pCam.negYCam.FarPlane,
				pCam.negYCam.FOV,
				pCam.negYCam.AspectRatio);
		pCam.negYCam.WVPMatrix = pCam.negYCam.ProjectionMatrix * pCam.negYCam.ViewMatrix;

		pCam.negZCam.Position = floral::vec3f(0.0f, 0.0f, 0.0f);
		pCam.negZCam.LookAtDir = floral::vec3f(0.0f, 0.0f, -1.0f);
		pCam.negZCam.AspectRatio = 1.0f;
		pCam.negZCam.NearPlane = 0.01f;
		pCam.negZCam.FarPlane = 1000.0f;
		pCam.negZCam.FOV = 90.0f;
		pCam.negZCam.ViewMatrix = floral::construct_lookat(
				floral::vec3f(0.0f, 1.0f, 0.0f),
				pCam.negZCam.Position,
				pCam.negZCam.LookAtDir);
		pCam.negZCam.ProjectionMatrix = floral::construct_perspective(
				pCam.negZCam.NearPlane,
				pCam.negZCam.FarPlane,
				pCam.negZCam.FOV,
				pCam.negZCam.AspectRatio);
		pCam.negZCam.WVPMatrix = pCam.negZCam.ProjectionMatrix * pCam.negZCam.ViewMatrix;
		m_ProbeCameras.push_back(pCam);
	}

	m_IsReady = true;
}

void ProbesBaker::Render()
{
	if (m_IsReady) {
		insigne::begin_render_pass(m_ProbesFramebuffer, 128 * 0, 0, 128, 128);
		m_Game->RenderWithCamera(&(m_ProbeCameras[0].posXCam));
		insigne::end_render_pass(m_ProbesFramebuffer);
		insigne::dispatch_render_pass();

		insigne::begin_render_pass(m_ProbesFramebuffer, 128 * 1, 0, 128, 128);
		m_Game->RenderWithCamera(&(m_ProbeCameras[0].negXCam));
		insigne::end_render_pass(m_ProbesFramebuffer);
		insigne::dispatch_render_pass();

		insigne::begin_render_pass(m_ProbesFramebuffer, 128 * 2, 0, 128, 128);
		m_Game->RenderWithCamera(&(m_ProbeCameras[0].posYCam));
		insigne::end_render_pass(m_ProbesFramebuffer);
		insigne::dispatch_render_pass();

		insigne::begin_render_pass(m_ProbesFramebuffer, 128 * 3, 0, 128, 128);
		m_Game->RenderWithCamera(&(m_ProbeCameras[0].negYCam));
		insigne::end_render_pass(m_ProbesFramebuffer);
		insigne::dispatch_render_pass();

		insigne::begin_render_pass(m_ProbesFramebuffer, 128 * 4, 0, 128, 128);
		m_Game->RenderWithCamera(&(m_ProbeCameras[0].posZCam));
		insigne::end_render_pass(m_ProbesFramebuffer);
		insigne::dispatch_render_pass();

		insigne::begin_render_pass(m_ProbesFramebuffer, 128 * 5, 0, 128, 128);
		m_Game->RenderWithCamera(&(m_ProbeCameras[0].negZCam));
		insigne::end_render_pass(m_ProbesFramebuffer);
		insigne::dispatch_render_pass();
	}
}

insigne::framebuffer_handle_t ProbesBaker::GetMegaFramebuffer()
{
	return m_ProbesFramebuffer;
}

}
