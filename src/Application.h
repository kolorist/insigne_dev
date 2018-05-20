#pragma once

#include "Logic/Game.h"
#include "Graphics/ITextureManager.h"
#include "Graphics/IModelManager.h"

namespace stone {
	class Application {
		public:
			Application();
			~Application();

		private:
			Game								m_Game;
			ITextureManager*					m_TextureManager;
			IModelManager*						m_ModelManager;
	};
}
