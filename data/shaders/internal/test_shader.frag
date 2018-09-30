#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec3 v_Normal_W;
in mediump vec2 v_TexCoord;

uniform sampler2D iu_Tex0;
uniform sampler2D iu_Tex1;
uniform samplerCube iu_CubeTex0;

void main()
{
	mediump vec3 c0 = texture(iu_Tex0, v_TexCoord).rgb;
	mediump vec3 c1 = texture(iu_Tex1, v_TexCoord).rgb;
	
	mediump vec3 c2 = texture(iu_CubeTex0, v_Normal_W).rgb;
	o_Color = vec4(c0 + c1 + c2, 1.0);
}