#version 300 es

layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

uniform highp mat4 iu_PerspectiveWVP;

out mediump vec4 v_Color;

void main() {
	v_Color = l_Color;
	gl_Position = iu_PerspectiveWVP * vec4(l_Position_L, 1.0f);
}
