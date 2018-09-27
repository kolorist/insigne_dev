#include "LightingManager.h"

#include <context.h>
#include <insigne/render.h>

#include "MaterialManager.h"
#include "Camera.h"
#include "Logic/Game.h"

namespace stone {

LightingManager::LightingManager(Game* i_game, MaterialManager* i_materialManager)
	: m_Game(i_game)
	, m_MaterialManager(i_materialManager)
	, m_IsReady(false)
{
}

LightingManager::~LightingManager()
{
}

void LightingManager::Initialize()
{
	{
		insigne::framebuffer_descriptor_t depthFbDesc = insigne::create_framebuffer_descriptor(0);
		depthFbDesc.has_depth = true;
		depthFbDesc.width = 1024 * 6;
		depthFbDesc.height = 1024;

		insigne::framebuffer_handle_t depthFb = insigne::create_framebuffer(depthFbDesc);
		m_DepthRenderFb = depthFb;
	}

	{
		m_DepthRenderMat = m_MaterialManager->CreateMaterialFromFile(floral::path("gfx/mat/depth_render.mat"));
	}

	{
		floral::vec3f lightPosition(3.0f, 0.5f, 0.3f);
		// init omni camera descriptors
		// positive x
		m_LightCamera.ViewDesc[0].position = lightPosition;
		m_LightCamera.ViewDesc[0].look_at = floral::vec3f(1.0f, 0.0f, 0.0f);
		m_LightCamera.ViewDesc[0].up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);
		// negative x
		m_LightCamera.ViewDesc[1].position = lightPosition;
		m_LightCamera.ViewDesc[1].look_at = floral::vec3f(-1.0f, 0.0f, 0.0f);
		m_LightCamera.ViewDesc[1].up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);
		// positive y
		m_LightCamera.ViewDesc[2].position = lightPosition;
		m_LightCamera.ViewDesc[2].look_at = floral::vec3f(0.0f, 1.0f, 0.0f);
		m_LightCamera.ViewDesc[2].up_direction = floral::vec3f(0.0f, 0.0f, 1.0f);
		// negative y
		m_LightCamera.ViewDesc[3].position = lightPosition;
		m_LightCamera.ViewDesc[3].look_at = floral::vec3f(0.0f, -1.0f, 0.0f);
		m_LightCamera.ViewDesc[3].up_direction = floral::vec3f(0.0f, 0.0f, -1.0f);
		// positive z
		m_LightCamera.ViewDesc[4].position = lightPosition;
		m_LightCamera.ViewDesc[4].look_at = floral::vec3f(0.0f, 0.0f, -1.0f);
		m_LightCamera.ViewDesc[4].up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);
		// negative z
		m_LightCamera.ViewDesc[5].position = lightPosition;
		m_LightCamera.ViewDesc[5].look_at = floral::vec3f(0.0f, 0.0f, 1.0f);
		m_LightCamera.ViewDesc[5].up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

		m_LightCamera.PerspDesc.near_plane = 0.01f;
		m_LightCamera.PerspDesc.far_plane = 1000.0f;
		m_LightCamera.PerspDesc.fov = 90.0f;
		m_LightCamera.PerspDesc.aspect_ratio = 1.0f;
		floral::mat4x4f proj = floral::construct_perspective(m_LightCamera.PerspDesc);
		for (u32 i = 0; i < 6; i++) {
			floral::mat4x4f view = floral::construct_lookat_dir(m_LightCamera.ViewDesc[i]);
			m_LightCamera.ViewMatrix[i] = view;
			m_LightCamera.ViewProjMatrix[i] = proj * view;
		}
	}

	m_IsReady = true;
}

void LightingManager::RenderShadowMap()
{
	if (!m_IsReady)
		return;

	for (u32 i = 0; i < 6; i++) {
		Camera tmpCam;
		tmpCam.ViewMatrix = m_LightCamera.ViewMatrix[i];
		tmpCam.WVPMatrix = m_LightCamera.ViewProjMatrix[i];

		insigne::begin_render_pass(m_DepthRenderFb, 1024 * i, 0, 1024, 1024);
		m_Game->RenderWithMaterial(&tmpCam, m_DepthRenderMat);
		insigne::end_render_pass(m_DepthRenderFb);
		insigne::dispatch_render_pass();
	}
}

insigne::texture_handle_t LightingManager::GetShadowMap()
{
	return insigne::texture_handle_t();
}

}
