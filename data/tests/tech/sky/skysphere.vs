#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
	mediump float iu_roughness;
};

out highp vec3 v_Normal;

void main()
{
	v_Normal = l_Normal_L;
	highp vec4 posC = iu_viewProjectionMatrix * vec4(l_Position_L, 1.0f);
	gl_Position = posC;
}