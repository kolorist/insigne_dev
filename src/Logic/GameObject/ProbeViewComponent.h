#pragma once

#include <floral.h>

#include "Component.h"
#include "Graphics/Camera.h"

namespace stone {

class ProbeViewComponent : public Component {
	public:
		ProbeViewComponent();
		~ProbeViewComponent();

		void									Render();

		void									Initialize(const floral::vec3f& i_position);

	private:
		Camera									m_SideCamera;
};

}
