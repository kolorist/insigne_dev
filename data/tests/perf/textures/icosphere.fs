#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
	highp vec3 iu_cameraPosition;
	mediump vec3 iu_sh[9];
};

uniform mediump samplerCube u_MainTex;

in mediump vec3 v_SampleDir;

mediump vec3 DecodeRGBM(in mediump vec4 i_rgbmColor)
{
	mediump vec3 hdrColor = 6.0f * i_rgbmColor.rgb * i_rgbmColor.a;
	return (hdrColor * hdrColor);
}

void main()
{
	//mediump vec3 color = DecodeRGBM(texture(u_MainTex, v_SampleDir).rgba);
	mediump vec3 color = texture(u_MainTex, v_SampleDir).rgb;
	o_Color = vec4(color, 1.0f);
}