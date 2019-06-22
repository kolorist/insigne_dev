#pragma once

#include <floral.h>

#include <imgui.h>

#include <insigne/commons.h>
#include <insigne/ut_render.h>
#include <insigne/detail/rt_render.h>

#include <lotus/profiler.h>

#include "Memory/MemorySystem.h"

/*
 * FIXME:
 * - we have to disable unused attribute binding points because some of GPUs will perform funny without it
 */

namespace stone {

struct GeoQuad {
	floral::vec3f								Vertices[4];
	floral::vec3f								Normal;
	floral::vec4f								Color;
	floral::vec4f								RadiosityColor;
	floral::fixed_array<u32, LinearAllocator>	PatchLinks;
	floral::fixed_array<f32, LinearAllocator>	FormFactors;
};
// ---------------------------------------------

struct VertexP {
	floral::vec3f								Position;
};

struct SurfaceP {
	static u32 index;
	static const u32 draw_calls_budget = 512u;
	static const insigne::geometry_mode_e geometry_mode = insigne::geometry_mode_e::triangles;

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
		detail::describe_vertex_data(0, 3, data_type_e::elem_signed_float, false, sizeof(VertexP), (const voidptr)0);
	}
};

// --------------------------------------------

struct VertexPN {
	floral::vec3f								Position;
	floral::vec3f								Normal;
};

// ---------------------------------------------

struct VertexPT
{
	floral::vec2f								Position;
	floral::vec2f								TexCoord;
};

struct SurfacePT
{
	static u32 index;
	static const u32 draw_calls_budget = 16u;
	static const insigne::geometry_mode_e geometry_mode = insigne::geometry_mode_e::triangles;

	static void setup_states()
	{
		using namespace insigne;
		detail::set_blending<false_type>(blend_equation_e::func_add, factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
		detail::set_cull_face<true_type>(face_side_e::back_side, front_face_e::face_ccw);
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
		detail::describe_vertex_data(0, 2, data_type_e::elem_signed_float, false, sizeof(VertexPT), (const voidptr)0);
		detail::describe_vertex_data(1, 2, data_type_e::elem_signed_float, false, sizeof(VertexPT), (const voidptr)8);
	}
};

// ---------------------------------------------
struct VertexPC {
	floral::vec3f								Position;
	floral::vec4f								Color;
};

struct SurfacePC {
	static u32 index;
	static const u32 draw_calls_budget = 64u;
	static const insigne::geometry_mode_e geometry_mode = insigne::geometry_mode_e::triangles;

	static void setup_states()
	{
		using namespace insigne;
		detail::set_blending<false_type>(blend_equation_e::func_add, factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
		detail::set_cull_face<false_type>(face_side_e::back_side, front_face_e::face_ccw);
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
		detail::describe_vertex_data(0, 3, data_type_e::elem_signed_float, false, sizeof(VertexPC), (const voidptr)0);
		detail::describe_vertex_data(1, 4, data_type_e::elem_signed_float, false, sizeof(VertexPC), (const voidptr)12);
	}
};

struct DebugLine {
	static u32 index;
	static const u32 draw_calls_budget = 256u;
	static const insigne::geometry_mode_e geometry_mode = insigne::geometry_mode_e::lines;

	static void setup_states()
	{
		using namespace insigne;
		detail::set_blending<false_type>(blend_equation_e::func_add, factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
		detail::set_cull_face<false_type>(face_side_e::back_side, front_face_e::face_ccw);
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
		detail::describe_vertex_data(0, 3, data_type_e::elem_signed_float, false, sizeof(VertexPC), (const voidptr)0);
		detail::describe_vertex_data(1, 4, data_type_e::elem_signed_float, false, sizeof(VertexPC), (const voidptr)12);
	}
};

// ---------------------------------------------
struct VertexPNC {
	floral::vec3f								Position;
	floral::vec3f								Normal;
	floral::vec4f								Color;
};

struct SurfacePNC {
	static u32 index;
	static const u32 draw_calls_budget = 64u;
	static const insigne::geometry_mode_e geometry_mode = insigne::geometry_mode_e::triangles;

	static void setup_states()
	{
		using namespace insigne;
		detail::set_blending<false_type>(blend_equation_e::func_add, factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
		detail::set_cull_face<false_type>(face_side_e::back_side, front_face_e::face_ccw);
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
		detail::enable_vertex_attrib(2);
		detail::describe_vertex_data(0, 3, data_type_e::elem_signed_float, false, sizeof(VertexPNC), (const voidptr)0);
		detail::describe_vertex_data(1, 3, data_type_e::elem_signed_float, false, sizeof(VertexPNC), (const voidptr)12);
		detail::describe_vertex_data(2, 4, data_type_e::elem_signed_float, false, sizeof(VertexPNC), (const voidptr)24);
	}
};

// ---------------------------------------------
struct VertexPNCC {
	floral::vec3f								Position;
	floral::vec3f								Normal;
	floral::vec4f								Color0;
	floral::vec4f								Color1;
};

struct SurfacePNCC {
	static u32 index;
	static const insigne::geometry_mode_e geometry_mode = insigne::geometry_mode_e::triangles;

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
		detail::enable_vertex_attrib(2);
		detail::enable_vertex_attrib(3);
		detail::describe_vertex_data(0, 3, data_type_e::elem_signed_float, false, sizeof(VertexPNCC), (const voidptr)0);
		detail::describe_vertex_data(1, 3, data_type_e::elem_signed_float, false, sizeof(VertexPNCC), (const voidptr)12);
		detail::describe_vertex_data(2, 4, data_type_e::elem_signed_float, false, sizeof(VertexPNCC), (const voidptr)24);
		detail::describe_vertex_data(3, 4, data_type_e::elem_signed_float, false, sizeof(VertexPNCC), (const voidptr)40);
	}
};

// ---------------------------------------------
struct VertexPNCSH {
	floral::vec3f								Position;
	floral::vec3f								Normal;
	floral::vec4f								Color;
	floral::vec3f								SH[9];
};

struct SurfacePNCSH {
	static u32 index;
	static const u32 draw_calls_budget = 64u;
	static const insigne::geometry_mode_e geometry_mode = insigne::geometry_mode_e::triangles;

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
		detail::enable_vertex_attrib(2);

		for (u32 i = 0; i < 9; i++)
		{
			detail::enable_vertex_attrib(3 + i);
		}

		detail::describe_vertex_data(0, 3, data_type_e::elem_signed_float, false, sizeof(VertexPNCSH), (const voidptr)0);
		detail::describe_vertex_data(1, 3, data_type_e::elem_signed_float, false, sizeof(VertexPNCSH), (const voidptr)12);
		detail::describe_vertex_data(2, 4, data_type_e::elem_signed_float, false, sizeof(VertexPNCSH), (const voidptr)24);

		for (u32 i = 0; i < 9; i++)
		{
			detail::describe_vertex_data(3 + i, 3, data_type_e::elem_signed_float, false, sizeof(VertexPNCSH), (const voidptr)(40 + 12 * i));
		}
	}
};

// ---------------------------------------------
struct ImGuiVertex {
	floral::vec2f								Position;
	floral::vec2f								TexCoord;
	u32											Color;
};

struct ImGuiSurface {
	static u32 index;
	static const u32 draw_calls_budget = 64u;
	static const insigne::geometry_mode_e geometry_mode = insigne::geometry_mode_e::triangles;

	static void setup_states()
	{
		using namespace insigne;
		detail::set_blending<true_type>(blend_equation_e::func_add, factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
		detail::set_cull_face<false_type>(face_side_e::back_side, front_face_e::face_ccw);
		detail::set_depth_test<false_type>(compare_func_e::func_less_or_equal);
		detail::set_depth_write<false_type>();
	}

	static void describe_vertex_data()
	{
		using namespace insigne;

		// vertex attributes
		detail::enable_vertex_attrib(0);
		detail::enable_vertex_attrib(1);
		detail::enable_vertex_attrib(2);
		detail::describe_vertex_data(0, 2, data_type_e::elem_signed_float, false, sizeof(ImGuiVertex), (const voidptr)0);
		detail::describe_vertex_data(1, 2, data_type_e::elem_signed_float, false, sizeof(ImGuiVertex), (const voidptr)8);
		detail::describe_vertex_data(2, 4, data_type_e::elem_unsigned_byte, false, sizeof(ImGuiVertex), (const voidptr)16);
	}
};

}
