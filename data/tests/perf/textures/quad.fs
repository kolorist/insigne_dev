#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Blit
{
	mediump vec3 iu_OverlayColor;
};

uniform mediump sampler2D u_MainTex;

in mediump vec2 v_TexCoord;

mediump vec3 DecodeRGBM(in mediump vec4 i_rgbmColor)
{
	mediump vec3 hdrColor = 6.0f * i_rgbmColor.rgb * i_rgbmColor.a;
	return (hdrColor * hdrColor);
}

void main()
{
	//mediump vec3 mainColor = DecodeRGBM(texture(u_MainTex, v_TexCoord).rgba);
	mediump vec3 mainColor = texture(u_MainTex, v_TexCoord).rgb;
	mainColor = pow(mainColor, vec3(0.454545f));
	o_Color = vec4(mainColor, 1.0f);
}