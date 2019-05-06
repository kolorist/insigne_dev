#include "GILightProbe.h"

#include <floral/io/nativeio.h>

#include <clover/Logger.h>

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include "Graphics/GeometryBuilder.h"

namespace stone
{

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

out mediump vec4 v_VertexColor;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_VertexColor = l_Color;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_VertexColor;

void main()
{
	o_Color = v_VertexColor;
}
)";

}
