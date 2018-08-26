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

struct MaterialDesc {
	floral::path								cbShaderPath;
};

}
