#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

uniform highp mat4 iu_PerspectiveWVP;
uniform highp mat4 iu_TransformMat;

void main() {
	highp vec4 pos_W = iu_TransformMat * vec4(l_Position_L, 1.0f);
	gl_Position = iu_PerspectiveWVP * pos_W;
}
