#pragma once

#include <floral.h>

#include <insigne/render.h>
#include <insigne/renderer.h>

namespace stone {
	struct ImGuiSurface : insigne::renderable_surface_t<ImGuiSurface> {
		static void setup_states() {
			using namespace insigne;
			// setup states
			renderer::set_blending<true_type>(blend_equation_e::func_add, factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
			//renderer::set_cull_face<true_type>(front_face_e::face_ccw);
			renderer::set_depth_test<false_type>(compare_func_e::func_always);
			renderer::set_depth_write<false_type>();
		}
	};

	struct Vertex {
		floral::vec3f							Position;
		floral::vec3f							Normal;
		floral::vec2f							TexCoord;
	};

	struct SolidSurface : insigne::renderable_surface_t<SolidSurface> {
		static void setup_states() {
			using namespace insigne;
			renderer::set_blending<false_type>(blend_equation_e::func_add, factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
			//renderer::set_cull_face<true_type>(front_face_e::face_ccw);
			renderer::set_depth_test<true_type>(compare_func_e::func_less_or_equal);
			renderer::set_depth_write<true_type>();
		}
	};
}
