#version 300 es
layout (location = 0) in highp vec2 l_Position_L;
layout (location = 1) in mediump vec2 l_TexCoord;

out mediump vec2 o_TexCoord;

void main() {
	o_TexCoord = l_TexCoord;
	gl_Position = vec4(l_Position_L.xy, 0.0, 1.0f);
}