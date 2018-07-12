#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

uniform highp mat4 iu_TransformMat;
uniform highp mat4 iu_PerspectiveWVP;

out mediump vec3 v_Position_W;

void main() {
	highp vec4 pos_W = iu_TransformMat * vec4(l_Position_L, 1.0f);
	v_Position_W = l_Position_L;
	gl_Position = iu_PerspectiveWVP * pos_W;
}