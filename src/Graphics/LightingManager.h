#pragma once

#include <floral.h>

#include "ILightingManager.h"

namespace stone {

class Game;
class MaterialManager;

class LightingManager : public ILightingManager {
	private:
		struct OmniCamera {
			floral::camera_view_t				ViewDesc[6];
			floral::camera_persp_t				PerspDesc;
			floral::mat4x4f						ViewMatrix[6];
			floral::mat4x4f						ViewProjMatrix[6];
		};

	public:
		LightingManager(Game* i_game, MaterialManager* i_materialManager);
		~LightingManager();

		void									Initialize() override;
		void									RenderShadowMap() override;

		insigne::texture_handle_t				GetShadowMap() override;

	private:
		Game*									m_Game;
		MaterialManager*						m_MaterialManager;
		bool									m_IsReady;

		OmniCamera								m_LightCamera;

		insigne::material_handle_t				m_DepthRenderMat;
		insigne::framebuffer_handle_t			m_DepthRenderFb;
};

}
