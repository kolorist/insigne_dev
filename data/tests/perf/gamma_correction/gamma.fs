#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Material
{
	mediump vec3 iu_Color;
};

in mediump vec2 v_TexCoord;

void main()
{
	if (v_TexCoord.x < 0.5f)
	{
		if (mod(gl_FragCoord.x + gl_FragCoord.y, 2.0f) < 1.0f)
		{
			o_Color = vec4(iu_Color, 1.0f);
		}
		else
		{
			o_Color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
		}
	}
	else
	{
		mediump vec3 halfColor = iu_Color * 0.5f;
		halfColor = pow(halfColor, vec3(1.0f / 2.2f));
		o_Color = vec4(halfColor, 1.0f);
	}
}