#pragma once

#include <floral.h>

#include <imgui.h>

#include <insigne/commons.h>
#include <insigne/render.h>
#include <insigne/renderer.h>

#include <lotus/profiler.h>

/*
 * FIXME:
 * - we have to disable unused attribute binding points because some of GPUs will perform funny without it
 */

namespace stone {

struct DemoVertex {
	floral::vec3f								Position;
	floral::vec4f								Color;
};

struct DemoSurface : insigne::renderable_surface_t<DemoSurface> {
	static const insigne::geometry_mode_e s_geometry_mode = insigne::geometry_mode_e::triangles;

	static void setup_states()
	{
		using namespace insigne;
		renderer::set_blending<false_type>(blend_equation_e::func_add, factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
		renderer::set_cull_face<false_type>(face_side_e::back_side, front_face_e::face_ccw);
		renderer::set_depth_test<false_type>(compare_func_e::func_less_or_equal);
		renderer::set_depth_write<false_type>();
		renderer::set_scissor_test<false_type>(0, 0, 0, 0);

		// vertex attributes
		renderer::enable_vertex_attrib(0);
		renderer::enable_vertex_attrib(1);
	}

	static void describe_vertex_data()
	{
		using namespace insigne;
		renderer::describe_vertex_data(0, 3, data_type_e::elem_signed_float, false, sizeof(DemoVertex), (const voidptr)0);
		renderer::describe_vertex_data(1, 4, data_type_e::elem_signed_float, false, sizeof(DemoVertex), (const voidptr)12);
	}
};

}
