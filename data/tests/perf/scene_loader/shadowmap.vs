#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
};

void main()
{
	vec3 posW = l_Position_L;
	gl_Position = iu_viewProjectionMatrix * vec4(posW, 1.0f);
}