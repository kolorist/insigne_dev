#version 300 es
layout (location = 0) in highp vec2 l_Position_L;
layout (location = 1) in highp vec2 l_TexCoord;

out highp vec2 v_TexCoord;

void main()
{
	v_TexCoord = l_TexCoord;
	gl_Position = vec4(l_Position_L, 0.0f, 1.0f);
}

