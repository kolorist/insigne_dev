#pragma once

#include <floral.h>
#include <insigne/commons.h>

namespace cymbi {

struct SurfaceDesc {
};

struct ModelDesc {
};

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

	floral::inplace_array<ParamNVP<floral::path>, 4u>	tex2DParams;
	floral::inplace_array<ParamNVP<floral::path>, 4u>	texCubeParams;
	floral::inplace_array<ParamNVP<f32>, 8u>			floatParams;
};

}
