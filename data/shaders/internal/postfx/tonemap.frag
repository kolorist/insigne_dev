#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 o_TexCoord;

uniform mediump sampler2D iu_ColorTex0;
uniform mediump float iu_Exposure;

void main()
{
	mediump vec4 hdrColor = texture(iu_ColorTex0, o_TexCoord).rgba;
	
	// exposure tone mapping
	o_Color = vec4(1.0) - exp(-hdrColor * iu_Exposure);
}