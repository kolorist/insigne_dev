#pragma once

#include <stdint.h>
#include <string.h>         					// memset, memmove, memcpy, strlen, strchr, strcpy, strcmp
#include <math.h>

#ifndef SHAPEGEN_ASSERT
#include <assert.h>
#define SHAPEGEN_ASSERT(_EXPR)    assert(_EXPR)
#endif

#define SHAPEGEN_ALLOC(X)						get_io().alloc(X)
#define SHAPEGEN_FREE(X)						get_io().free(X)

namespace shapegen
{

struct vec3f
{
	float x, y, z;
};

inline vec3f add(const vec3f& i_a, const vec3f& i_b)
{
	return vec3f {
		i_a.x + i_b.x, i_a.y + i_b.y, i_a.z + i_b.z
	};
}

inline vec3f cross(const vec3f& i_a, const vec3f& i_b)
{
	return vec3f {
		i_a.y * i_b.z - i_a.z * i_b.y,
		i_a.z * i_b.x - i_a.x * i_b.z,
		i_a.x * i_b.y - i_a.y * i_b.x
	};
}

inline float length(const vec3f& i_a)
{
	return sqrtf(i_a.x * i_a.x + i_a.y * i_a.y + i_a.z * i_a.z);
}

inline vec3f normalize(const vec3f& i_a)
{
	float len = length(i_a);
	SHAPEGEN_ASSERT(len > 0.0f);
	return vec3f { i_a.x / len, i_a.y / len, i_a.z / len };
}

//----------------------------------------------

struct color4f
{
	float r, g, b, a;
};

//----------------------------------------------

template <class data_t>
class dynamic_array
{
	typedef data_t								value_type;
	typedef value_type*							pointer_type;
	typedef value_type&							reference_type;
	typedef const value_type&					const_reference_type;

public:
	inline dynamic_array()
		: data(nullptr)
		, capacity(0)
		, current_size(0)
	{ }

	inline ~dynamic_array()
	{
		if (data)
		{
			SHAPEGEN_FREE(data);
		}
	}

	inline value_type& operator[](size_t i_idx)
	{
		SHAPEGEN_ASSERT(i_idx < current_size);
		return data[i_idx];
	}

	inline size_t get_size() const
	{
		return current_size;
	}

	inline void resize(size_t i_newSize)
	{
		if (i_newSize > capacity)
		{
			reserve(i_newSize);
		}

		current_size = i_newSize;
	}

	inline void clear()
	{
		current_size = 0;
		capacity = 0;
		if (data)
		{
			SHAPEGEN_FREE(data);
		}
	}

	inline void reserve(size_t i_newCapacity)
	{
		if (i_newCapacity < capacity)
			return;

		size_t po2Capacity = i_newCapacity;
		po2Capacity--;
		po2Capacity |= po2Capacity >> 1;
		po2Capacity |= po2Capacity >> 2;
		po2Capacity |= po2Capacity >> 4;
		po2Capacity |= po2Capacity >> 8;
		po2Capacity |= po2Capacity >> 16;
		po2Capacity++;

		pointer_type newData = (pointer_type)SHAPEGEN_ALLOC(po2Capacity * sizeof(value_type));
		if (data)
		{
			memcpy(newData, data, capacity * sizeof(value_type));
			SHAPEGEN_FREE(data);
		}
		data = newData;
		capacity = po2Capacity;
	}

	inline void push_back(const_reference_type i_data)
	{
		if (current_size == capacity)
		{
			reserve(current_size + 1);
		}

		data[current_size] = i_data;
		current_size++;
	}

private:
	pointer_type								data;
	size_t										capacity;
	size_t										current_size;
};

//----------------------------------------------

enum class data_scheme_e
{
	invalid = 0,
	separate,
	non_interleave,
	interleave
};

enum class vertex_format_e
{
	invalid										= 0,
	position									= 1u << 1,
	normal										= 1u << 2,
	color										= 1u << 3,
	tex_coord									= 1u << 4
};

//----------------------------------------------

struct generator_io_t
{
	void*										(*alloc)(size_t);
	void										(*free)(void*);

	data_scheme_e								data_scheme;
	vertex_format_e								vertex_format;
	color4f										current_vertex_color;
};

struct geometry_soup_t
{
	float*										position;
	float*										normal;
	float*										color;
	float*										tex_coord;

	float*										vertices;

	int32_t*									indices;

	size_t										vertices_count;
	size_t										indices_count;
};

//----------------------------------------------

generator_io_t&									get_io();
void											initialize();

void											begin_generation(const data_scheme_e i_dataScheme, const vertex_format_e i_vertexFormat);
void											end_generation();
const geometry_soup_t&							get_result_geometry();

//----------------------------------------------

void											set_vertex_color(const color4f& i_color);

// normal = cross(i_u, i_v);
void											push_plane_3d(const vec3f& i_origin, const vec3f& i_u, const vec3f& i_v);
//void											push_box_3d(const vec3f& i_minCorner, const vec3f& i_maxCorner);

}
