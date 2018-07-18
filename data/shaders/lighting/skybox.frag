#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec3 v_Position_W;

uniform mediump samplerCube iu_TexBaseColor;

void main()
{
	mediump vec3 sampleDir = vec3(v_Position_W.x, v_Position_W.y, v_Position_W.z);
	mediump vec3 outColor = textureLod(iu_TexBaseColor, sampleDir, 0.0f).rgb;
	o_Color = vec4(vec3(outColor), 1.0f);
}