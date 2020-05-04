#version 300 es
layout (location = 0) out mediump vec3 o_Color;

uniform mediump sampler2D u_Tex;
in mediump vec3 v_Normal;

mediump vec3 textureLatLong(in sampler2D i_tex, in mediump vec3 i_sampleDir)
{
	mediump float phi = atan(i_sampleDir.x, i_sampleDir.z);
	mediump float theta = acos(i_sampleDir.y);

	mediump vec2 uv = vec2((3.14f + phi) * 0.15923f, theta * 0.31847f);
	return texture(i_tex, uv).rgb;
}

void main()
{
	mediump vec3 sampleDir = normalize(v_Normal);
	mediump vec3 outColor = textureLatLong(u_Tex, sampleDir);
	o_Color = outColor;
}