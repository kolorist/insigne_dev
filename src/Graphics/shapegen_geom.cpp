#include "shapegen.h"

#define PUSH_BACK_VEC3F(ARRAY, VEC)				ARRAY.push_back(VEC.x); ARRAY.push_back(VEC.y); ARRAY.push_back(VEC.z)

namespace shapegen
{

static geometry_soup_t							s_geometry_soup;

static dynamic_array<float>						s_positions;
static dynamic_array<float>						s_normals;
static dynamic_array<float>						s_colors;
static dynamic_array<float>						s_texcoords;

static dynamic_array<float>						s_vertices;

static dynamic_array<int32_t>					s_indices;

void begin_generation(const data_scheme_e i_dataScheme, const vertex_format_e i_vertexFormat)
{
	get_io().data_scheme = i_dataScheme;
	get_io().vertex_format = i_vertexFormat;

	memset(&s_geometry_soup, 0, sizeof(geometry_soup_t));
	s_positions.resize(0);
	s_normals.resize(0);
	s_colors.resize(0);
	s_texcoords.resize(0);
	s_vertices.resize(0);
	s_indices.resize(0);
}

void end_generation()
{
	s_geometry_soup.position = &s_positions[0];
	s_geometry_soup.indices = &s_indices[0];
}

const geometry_soup_t& get_result_geometry()
{
	return s_geometry_soup;
}

//----------------------------------------------

void set_vertex_color(const color4f& i_color)
{
	get_io().current_vertex_color = i_color;
}

//----------------------------------------------

void push_plane_3d(const vec3f& i_origin, const vec3f& i_u, const vec3f& i_v)
{
	vec3f v0 = i_origin;
	vec3f v1 = add(v0, i_v);
	vec3f v2 = add(v1, i_u);
	vec3f v3 = add(v0, i_u);

	int32_t currentIdx = (int32_t)s_positions.get_size();

	PUSH_BACK_VEC3F(s_positions, v0);
	PUSH_BACK_VEC3F(s_positions, v1);
	PUSH_BACK_VEC3F(s_positions, v2);
	PUSH_BACK_VEC3F(s_positions, v3);

	s_indices.push_back(currentIdx + 0);
	s_indices.push_back(currentIdx + 1);
	s_indices.push_back(currentIdx + 2);
	s_indices.push_back(currentIdx + 2);
	s_indices.push_back(currentIdx + 3);
	s_indices.push_back(currentIdx + 0);

	s_geometry_soup.vertices_count += 4;
	s_geometry_soup.indices_count += 6;
}

}
