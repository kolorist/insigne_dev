#pragma once

#include "Logic/Game.h"
#include "Graphics/ITextureManager.h"
#include "Graphics/IModelManager.h"
#include "Graphics/IShaderManager.h"
#include "Graphics/MaterialManager.h"
#include "System/Controller.h"
#include "ImGuiDebug/Debugger.h"

namespace stone {
	class Application {
		public:
			Application(Controller* i_controller);
			~Application();

		private:
			void								UpdateFrame(f32 i_deltaMs);
			void								RenderFrame(f32 i_deltaMs);

			void								OnInitialize(int i_param);
			void								OnFrameStep(f32 i_deltaMs);
			void								OnCleanUp(int i_param);

			// user interactions
			void								OnCharacterInput(c8 i_character);
			void								OnCursorMove(u32 i_x, u32 i_y);
			void								OnCursorInteract(bool i_pressed, u32 i_buttonId);

		private:
			Game*								m_Game;
			IShaderManager*						m_ShaderManager;
			ITextureManager*					m_TextureManager;
			IModelManager*						m_ModelManager;
			MaterialManager*					m_MaterialManager;
			Debugger*							m_Debugger;
	};
}
