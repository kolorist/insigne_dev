#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
	mediump vec3 iu_cameraPos;
	mediump vec3 iu_sh[9];
};

in highp vec3 v_Normal_W;

void main()
{
	mediump vec3 n = normalize(v_Normal_W);

	o_Color = vec4(n, 1.0f);
}