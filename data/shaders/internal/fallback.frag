#version 300 es

layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D iu_TestTex;

in mediump vec2 o_TexCoord;

void main()
{
	//o_Color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
	o_Color = texture(iu_TestTex, o_TexCoord);
}