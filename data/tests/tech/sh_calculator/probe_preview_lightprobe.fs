#version 300 es
layout (location = 0) out mediump vec3 o_Color;

uniform mediump sampler2D u_Tex;
in mediump vec3 v_Normal;

mediump vec3 textureLightprobe(in sampler2D i_tex, in mediump vec3 i_sampleDir)
{
	mediump float d = sqrt(i_sampleDir.x * i_sampleDir.x + i_sampleDir.y * i_sampleDir.y);
	mediump float r = 0.15924f * acos(i_sampleDir.z) / max(d, 0.001f);
	mediump vec2 uv = vec2(0.5f + i_sampleDir.x * r, 0.5f - i_sampleDir.y * r);
	return texture(i_tex, uv).rgb;
}

void main()
{
	mediump vec3 sampleDir = normalize(v_Normal);
	mediump vec3 outColor = textureLightprobe(u_Tex, sampleDir);
	o_Color = outColor;
}