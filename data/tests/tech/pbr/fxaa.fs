#version 300 es
// https://github.com/mattdesl/glsl-fxaa/blob/master/fxaa.glsl
//#define USE_GREEN_AS_LUMA

layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 v_TexCoord;
in mediump vec2 v_TexCoord1;
in mediump vec2 v_TexCoord2;
in mediump vec2 v_TexCoord3;
in mediump vec2 v_TexCoord4;

uniform sampler2D u_MainTex;

layout(std140) uniform ub_FXAAConfigs
{
	mediump vec2 iu_TexelSize;
};

mediump float rgb2luma(in mediump vec3 rgb)
{
	return pow(dot(rgb, vec3(0.299f, 0.587f, 0.114f)), 1.0f / 2.2f);
}

const mediump float k_FxaaSpanMax = 16.0f;
const mediump float k_FxaaReduceMul = 0.25f * (1.0f / k_FxaaSpanMax);
const mediump float k_FxaaReduceMin = 1.0f / 128.0f;

void main()
{
	mediump vec4 texColor = texture(u_MainTex, v_TexCoord);
	mediump vec3 rgbM = texColor.rgb;
	mediump vec3 rgbNW = texture(u_MainTex, v_TexCoord1).rgb;
	mediump vec3 rgbNE = texture(u_MainTex, v_TexCoord2).rgb;
	mediump vec3 rgbSW = texture(u_MainTex, v_TexCoord3).rgb;
	mediump vec3 rgbSE = texture(u_MainTex, v_TexCoord4).rgb;

#ifdef USE_GREEN_AS_LUMA
	mediump float lumaNW = rgbNW.y;
	mediump float lumaNE = rgbNE.y;
	mediump float lumaSW = rgbSW.y;
	mediump float lumaSE = rgbSE.y;
	mediump float lumaM	 = rgbM.y;
#else
	mediump float lumaNW = rgb2luma(rgbNW);
	mediump float lumaNE = rgb2luma(rgbNE);
	mediump float lumaSW = rgb2luma(rgbSW);
	mediump float lumaSE = rgb2luma(rgbSE);
	mediump float lumaM	 = rgb2luma(rgbM);
#endif

	mediump float lumaMin = min(min(lumaNW, lumaNE), min(lumaSW, lumaSE));
	mediump float lumaMax = max(max(lumaNW, lumaNE), max(lumaSW, lumaSE));

	mediump vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y =	 ((lumaNW + lumaSW) - (lumaNE + lumaSE));

	mediump float dirReduce = max(
		(lumaNW + lumaNE + lumaSW + lumaSE) * k_FxaaReduceMul,
		k_FxaaReduceMin);
	mediump float rcpDirMin = 1.0f/(min(abs(dir.x), abs(dir.y)) + dirReduce);

	dir = min(vec2( k_FxaaSpanMax, k_FxaaSpanMax),
		  max(vec2(-k_FxaaSpanMax, -k_FxaaSpanMax),
		  dir * rcpDirMin)) * iu_TexelSize;

	mediump vec2 rgbAOffset0 = dir * (1.0f/3.0f - 0.5f);
	mediump vec2 rgbAOffset1 = dir * (2.0f/3.0f - 0.5f);

	mediump vec3 rgbA0 = texture(u_MainTex, v_TexCoord + rgbAOffset0).rgb;
	mediump vec3 rgbA1 = texture(u_MainTex, v_TexCoord + rgbAOffset1).rgb;
	mediump vec3 rgbA = (rgbA0 + rgbA1) * 0.5f;

	mediump vec2 rgbBOffset0 = dir * (0.0f/3.0f - 0.5f);
	mediump vec2 rgbBOffset1 = dir * (3.0f/3.0f - 0.5f);
	mediump vec3 rgbB0 = texture(u_MainTex, v_TexCoord + rgbBOffset0).rgb;
	mediump vec3 rgbB1 = texture(u_MainTex, v_TexCoord + rgbBOffset1).rgb;
	mediump vec3 rgbB = rgbA * 0.5f + 0.25f * ( rgbB0 + rgbB1);

#ifdef USE_GREEN_AS_LUMA
	mediump float lumaB = rgbB.y;
#else
	mediump float lumaB = rgb2luma(rgbB);
#endif

	mediump float t = max(1.0f - step(lumaMin, lumaB), 1.0f - step(lumaB, lumaMax));
	o_Color = vec4(mix(rgbB, rgbA, t), texColor.a);
}