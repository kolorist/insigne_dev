#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_Color;

void main()
{
	mediump vec4 v = vec4(0.0f, 1.0f, 0.0f, 0.0f);
	mediump mat4 m = mat4(
		0.0f, 1.0f, 2.0f, 3.0f,				// column 0
		4.0f, 5.0f, 6.0f, 7.0f,				// column 1
		8.0f, 9.0f, 10.0f, 11.0f,			// column 2
		12.0f, 13.0f, 14.0f, 15.0f);		// column 3

	mediump vec4 v0 = vec4(0.0f, 1.0f, 0.0f, 0.0f);
	mediump mat4x3 m0 = mat4x3(				// 4 columns 3 rows matrix
		0.0f, 1.0f, 2.0f,
		3.0f, 4.0f, 5.0f,
		6.0f, 7.0f,	8.0f,
		9.0f, 10.0f, 11.0f);
	mediump vec3 u0 = m0 * v0;
	o_Color = vec4(u0, 1.0f);
}