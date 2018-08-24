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

	floral::inplace_array<insigne::shader_param_t, 32u> shaderParams;
};

struct MaterialDesc {
	floral::path								shaderPath;
};

}
