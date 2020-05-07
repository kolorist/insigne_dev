#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D u_MainTex;

in mediump vec2 v_TexCoord;

mediump vec3 RRTAndODTFit(in mediump vec3 v)
{
	mediump vec3 a = v * (v + 0.0245786) - 0.000090537;
	mediump vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return a / b;
}

mediump vec3 ACESFitted(in mediump vec3 color)
{
	mediump mat3 ACESInputMat = mat3(
		0.59719, 0.07600, 0.02840,
		0.35458, 0.90834, 0.13383,
		0.04823, 0.01566, 0.83777
	);

	mediump mat3 ACESOutputMat = mat3(
		 1.60475, -0.10208, -0.00327,
		-0.53108,  1.10813, -0.07276,
		-0.07367, -0.00605,  1.07602
	);

    color = ACESInputMat * color;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = ACESOutputMat * color;
    return color;
}

const highp float A = 0.15f;
const highp float B = 0.50f;
const highp float C = 0.10f;
const highp float D = 0.20f;
const highp float E = 0.02f;
const highp float F = 0.30f;
const highp vec3 W = vec3(11.2f, 11.2f, 11.2f);

mediump vec3 Uncharted2Tonemap(in highp vec3 x)
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
	mediump vec3 mainColor = texture(u_MainTex, v_TexCoord).rgb;
#if 0
	mediump vec3 color = ACESFitted(mainColor);
#else
	mainColor *= 1.0f;
	mainColor = Uncharted2Tonemap(2.0f * mainColor);
	
	mediump vec3 whiteScale = 1.0f / Uncharted2Tonemap(W);
	mediump vec3 color = mainColor * whiteScale;
#endif
	color = pow(color, vec3(1.0f / 2.2f));
	o_Color = vec4(color, 1.0f);
}