#pragma once

#include "Callbacks.h"

namespace stone {

class Controller {
	public:
		Controller();
		~Controller();

	public:
		ControllerCallbacks						IOEvents;
};

}
