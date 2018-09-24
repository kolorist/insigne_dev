#pragma once

#include "ILightingManager.h"

namespace stone {

class Game;

class LightingManager : public ILightingManager {
	public:
		LightingManager(Game* i_game);
		~LightingManager();

		void									Initialize() override;
		void									RenderShadowMap() override;

		insigne::texture_handle_t				GetShadowMap() override;

	private:
		Game*									m_Game;
};

}
