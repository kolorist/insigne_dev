#pragma once

#include <floral.h>

#include <imgui.h>

#include <insigne/commons.h>
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

			// vertex attributes
			renderer::enable_vertex_attrib(0);
			renderer::enable_vertex_attrib(1);
			renderer::enable_vertex_attrib(2);
		}

		static void describe_vertex_data()
		{
			using namespace insigne;
			renderer::describe_vertex_data(0, 2, data_type_e::elem_signed_float, false, sizeof(ImDrawVert), (const voidptr)0);
			renderer::describe_vertex_data(1, 2, data_type_e::elem_signed_float, false, sizeof(ImDrawVert), (const voidptr)8);
			renderer::describe_vertex_data(2, 4, data_type_e::elem_unsigned_byte, true, sizeof(ImDrawVert), (const voidptr)16);
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
			renderer::set_depth_test<false_type>(compare_func_e::func_less_or_equal);
			renderer::set_depth_write<false_type>();

			// vertex attributes
			renderer::enable_vertex_attrib(0);
			renderer::enable_vertex_attrib(1);
			renderer::enable_vertex_attrib(2);
		}

		static void describe_vertex_data()
		{
			using namespace insigne;
			renderer::describe_vertex_data(0, 3, data_type_e::elem_signed_float, false, sizeof(Vertex), (const voidptr)0);
			renderer::describe_vertex_data(1, 3, data_type_e::elem_signed_float, false, sizeof(Vertex), (const voidptr)12);
			renderer::describe_vertex_data(2, 2, data_type_e::elem_signed_float, false, sizeof(Vertex), (const voidptr)24);
		}
	};
}
