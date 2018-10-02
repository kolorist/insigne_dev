#pragma once

#include <floral.h>

namespace stone {

class Controller;

class Application {
	public:
		Application(Controller* i_controller);
		~Application();

	private:
		void									UpdateFrame(f32 i_deltaMs);
		void									RenderFrame(f32 i_deltaMs);

		void									OnInitialize(int i_param);
		void									OnFrameStep(f32 i_deltaMs);
		void									OnCleanUp(int i_param);

		// user interactions
		void									OnCharacterInput(c8 i_character);
		void									OnKeyInput(u32 i_keyCode, u32 i_keyStatus);
		void									OnCursorMove(u32 i_x, u32 i_y);
		void									OnCursorInteract(bool i_pressed, u32 i_buttonId);
};

}
