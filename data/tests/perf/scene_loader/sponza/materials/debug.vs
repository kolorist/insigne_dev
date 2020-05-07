#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec2 l_TexCoord;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
	mediump vec3 iu_cameraPos;
	mediump vec3 iu_sh[9];
};

out highp vec3 v_Normal_W;

void main()
{
	v_Normal_W = l_Normal_L;
	vec3 posW = l_Position_L;
	gl_Position = iu_viewProjectionMatrix * vec4(posW, 1.0f);
}