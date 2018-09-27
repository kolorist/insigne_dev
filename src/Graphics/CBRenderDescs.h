#pragma once

#include <floral.h>
#include <insigne/commons.h>

namespace cymbi {

struct Vertex3D {
	floral::vec3f								position;
	floral::vec3f								normal;
	floral::vec2f								texCoord;
};

template <typename TAllocator>
struct SurfaceDesc {
	c8											name[256];

	floral::path								materialPath;
	floral::fixed_array<Vertex3D, TAllocator>	vertices;
	floral::fixed_array<u32, TAllocator>		indices;
};

template <typename TAllocator>
struct ModelDesc {
	c8											name[256];

	floral::fixed_array<SurfaceDesc<TAllocator>, TAllocator>	surfaces;
};

// ---------------------------------------------

struct ShaderDesc {
	floral::path								vertexShaderPath;
	floral::path								fragmentShaderPath;

	insigne::shader_param_list_t*				shaderParams;
};

template <typename T>
struct ParamNVP {
	c8											name[256];
	T											value;
};

struct MaterialDesc {
	floral::path										cbShaderPath;

	floral::inplace_array<ParamNVP<floral::path>, MAX_TEX2D_UNIFORMS>	tex2DParams;
	floral::inplace_array<ParamNVP<floral::path>, MAX_TEXCUBE_UNIFORMS>	texCubeParams;
	floral::inplace_array<ParamNVP<f32>, MAX_FLOAT_UNIFORMS>			floatParams;
	floral::inplace_array<ParamNVP<floral::vec3f>, MAX_VEC3_UNIFORMS>	vec3Params;
	floral::inplace_array<ParamNVP<insigne::ub_handle_t>, MAX_UNIFORM_BLOCKS>	ubParams;
};

}
