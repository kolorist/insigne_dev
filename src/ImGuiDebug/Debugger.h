#pragma once

#include <floral.h>
#include <imgui.h>

namespace stone {
	
	class Debugger {
		public:
			Debugger();
			~Debugger();

			void								Initialize();
			void								Update(f32 i_deltaMs);
			void								Render(f32 i_deltaMs);

			void								OnCharacterInput(c8 i_character);
			void								OnCursorMove(u32 i_x, u32 i_y);
			void								OnCursorInteract(bool i_pressed, u32 i_buttonId);

		private:
			f32									m_MouseX, m_MouseY;
			bool								m_MousePressed[2];
			bool								m_MouseHeldThisFrame[2];
	};

}
