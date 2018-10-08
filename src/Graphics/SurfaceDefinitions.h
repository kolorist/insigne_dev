#pragma once

#include <floral.h>

#include <imgui.h>

#include <insigne/commons.h>
#include <insigne/ut_render.h>
#include <insigne/detail/rt_render.h>

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
		detail::set_blending<false_type>(blend_equation_e::func_add, factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
		detail::set_cull_face<true_type>(face_side_e::back_side, front_face_e::face_ccw);
		detail::set_depth_test<true_type>(compare_func_e::func_less_or_equal);
		detail::set_depth_write<true_type>();
		detail::set_scissor_test<false_type>(0, 0, 0, 0);

	}

	static void describe_vertex_data()
	{
		using namespace insigne;

		// vertex attributes
		detail::enable_vertex_attrib(0);
		detail::enable_vertex_attrib(1);
		detail::describe_vertex_data(0, 3, data_type_e::elem_signed_float, false, sizeof(DemoVertex), (const voidptr)0);
		detail::describe_vertex_data(1, 4, data_type_e::elem_signed_float, false, sizeof(DemoVertex), (const voidptr)12);
	}
};

struct DemoTexturedVertex {
	floral::vec3f								Position;
	floral::vec2f								TexCoord;
};

struct DemoTexturedSurface : insigne::renderable_surface_t<DemoTexturedSurface> {
	static const insigne::geometry_mode_e s_geometry_mode = insigne::geometry_mode_e::triangles;

	static void setup_states()
	{
		using namespace insigne;
		detail::set_blending<false_type>(blend_equation_e::func_add, factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
		detail::set_cull_face<false_type>(face_side_e::back_side, front_face_e::face_ccw);
		detail::set_depth_test<false_type>(compare_func_e::func_less_or_equal);
		detail::set_depth_write<false_type>();
		detail::set_scissor_test<false_type>(0, 0, 0, 0);

	}

	static void describe_vertex_data()
	{
		using namespace insigne;

		// vertex attributes
		detail::enable_vertex_attrib(0);
		detail::enable_vertex_attrib(1);
		detail::describe_vertex_data(0, 3, data_type_e::elem_signed_float, false, sizeof(DemoTexturedVertex), (const voidptr)0);
		detail::describe_vertex_data(1, 2, data_type_e::elem_signed_float, false, sizeof(DemoTexturedVertex), (const voidptr)12);
	}
};

struct DebugVertex {
	floral::vec3f								Position;
	floral::vec4f								Color;
};

struct DebugLine : insigne::renderable_surface_t<DebugLine> {
	static const insigne::geometry_mode_e s_geometry_mode = insigne::geometry_mode_e::lines;

	static void setup_states()
	{
		using namespace insigne;
		detail::set_blending<false_type>(blend_equation_e::func_add, factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
		detail::set_cull_face<false_type>(face_side_e::back_side, front_face_e::face_ccw);
		detail::set_depth_test<false_type>(compare_func_e::func_less_or_equal);
		detail::set_depth_write<false_type>();
		detail::set_scissor_test<false_type>(0, 0, 0, 0);

	}

	static void describe_vertex_data()
	{
		using namespace insigne;

		// vertex attributes
		detail::enable_vertex_attrib(0);
		detail::enable_vertex_attrib(1);
		detail::describe_vertex_data(0, 3, data_type_e::elem_signed_float, false, sizeof(DebugVertex), (const voidptr)0);
		detail::describe_vertex_data(1, 4, data_type_e::elem_signed_float, false, sizeof(DebugVertex), (const voidptr)12);
	}
};

}
