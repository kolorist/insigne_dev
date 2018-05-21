#pragma once

#include <insigne/render.h>
#include <insigne/renderer.h>

namespace stone {
	struct ImGuiSurface : insigne::renderable_surface_t<ImGuiSurface> {
		static void setup_states() {

		}
	};

	struct SolidSurface : insigne::renderable_surface_t<SolidSurface> {
		static void setup_states() {
			
		}
	};
}
