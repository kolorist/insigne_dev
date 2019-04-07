#pragma once

#include <floral.h>

namespace stone {
	
struct ControllerCallbacks {
	floral::simple_callback<void>				OnInitialize;
	floral::simple_callback<void>				OnPause;
	floral::simple_callback<void>				OnResume;
	floral::simple_callback<void, bool>			OnFocusChanged;
	floral::simple_callback<void>				OnDisplayChanged;
	//--------------------------------------
	floral::simple_callback<void, f32>			OnFrameStep;
	floral::simple_callback<void, int>			OnCleanUp;
	// -------------------------------------
	floral::simple_callback<void, c8>			CharacterInput;
	floral::simple_callback<void, u32, u32>		KeyInput;
	floral::simple_callback<void, u32, u32>		CursorMove;
	floral::simple_callback<void, bool, u32>	CursorInteract;
};

}
