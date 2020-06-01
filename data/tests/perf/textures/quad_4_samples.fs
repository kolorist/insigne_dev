#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Blit
{
	mediump vec3 iu_OverlayColor;
	mediump vec2 iu_TexelSize;
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
	mediump vec3 mainColor = texture(u_MainTex, v_TexCoord).rgb;
	mediump vec3 c0 = texture(u_MainTex, vec2(v_TexCoord.x, v_TexCoord.y + iu_TexelSize.y)).rgb;
	mediump vec3 c1 = texture(u_MainTex, vec2(v_TexCoord.x, v_TexCoord.y - iu_TexelSize.y)).rgb;
	mediump vec3 c2 = texture(u_MainTex, vec2(v_TexCoord.x + iu_TexelSize.x, v_TexCoord.y)).rgb;
	mediump vec3 c3 = texture(u_MainTex, vec2(v_TexCoord.x + iu_TexelSize.y, v_TexCoord.y)).rgb;
	mediump vec3 col = mainColor + c0 + c1 + c2 + c3;
	col /= 5.0f;
	col = pow(col, vec3(0.454545f));
	o_Color = vec4(col, 1.0f);
}
