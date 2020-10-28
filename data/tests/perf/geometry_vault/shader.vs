#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
};

out mediump vec4 v_Color;

void main()
{
	v_Color = l_Color;
	gl_Position = iu_viewProjectionMatrix * vec4(l_Position_L, 1.0f);
}