#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_WVP;
	highp vec3 iu_CamPosition;
};

out highp vec3 v_Position;

void main()
{
	highp vec4 pos_W = iu_WVP * vec4(l_Position_L + iu_CamPosition, 1.0);
	gl_Position = pos_W.xyww;
	v_Position = l_Position_L;
}