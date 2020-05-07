#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
	mediump vec3 iu_cameraPos;
	mediump vec3 iu_sh[9];
};

uniform mediump sampler2D u_AlbedoTex;

in highp vec3 v_Normal_W;
in mediump vec2 v_TexCoord;

void main()
{
	mediump vec4 baseColor = texture(u_AlbedoTex, v_TexCoord);

	if (baseColor.a < 0.5f)
	{
		discard;
	}

	o_Color = baseColor;
}