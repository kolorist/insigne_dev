#version 300 es
layout (location = 0) in highp vec2 l_Position;
layout (location = 1) in mediump vec2 l_TexCoord;

out mediump vec2 v_TexCoord;
out mediump vec2 v_TexCoord1;
out mediump vec2 v_TexCoord2;
out mediump vec2 v_TexCoord3;
out mediump vec2 v_TexCoord4;

layout(std140) uniform ub_FXAAConfigs
{
	mediump vec2 iu_TexelSize;
};

const mediump float k_FxaaSubpixelShift = 0.25f;

void main()
{
	v_TexCoord = l_TexCoord;
	gl_Position = vec4(l_Position, 0.0f, 1.0f);

	mediump vec2 baseUV = l_TexCoord - (iu_TexelSize * (0.5f + k_FxaaSubpixelShift));
	v_TexCoord1 = baseUV;
	v_TexCoord2 = baseUV + vec2(1.0f, 0.0f) * iu_TexelSize;
	v_TexCoord3 = baseUV + vec2(0.0f, 1.0f) * iu_TexelSize;
	v_TexCoord4 = baseUV + vec2(1.0f, 1.0f) * iu_TexelSize;
}
