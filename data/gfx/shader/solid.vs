#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

void main()
{
	gl_Position = vec4(l_Position_L, 1.0f);
}