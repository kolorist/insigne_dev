#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
	highp vec3 iu_cameraPosition;
	mediump vec3 iu_sh[9];
};

out mediump vec3 v_Normal;

void main()
{
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_Normal = normalize(l_Position_L);
	gl_Position = iu_viewProjectionMatrix * pos_W;
}