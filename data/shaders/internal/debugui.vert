#version 300 es
layout (location = 0) in vec2 l_Position_L;
layout (location = 1) in vec2 l_TexCoord;
layout (location = 2) in vec4 l_VertColor;

uniform mat4 iu_DebugOrthoWVP;

out vec2 o_TexCoord;
out vec4 o_VertColor;

void main() {
	o_TexCoord = l_TexCoord;
	o_VertColor = l_VertColor;
	gl_Position = iu_DebugOrthoWVP * vec4(l_Position_L.xy, 0.0, 1.0f);
}