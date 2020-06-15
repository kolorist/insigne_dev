#version 300 es
layout (location = 0) out mediump vec3 o_Color;

uniform mediump sampler2D u_MainTex;
uniform mediump sampler2D u_BloomTex;

in mediump vec2 v_TexCoord;

const mediump float k_bloomIntensity = 2.0f;

const mediump float A = 0.15f;
const mediump float B = 0.50f;
const mediump float C = 0.10f;
const mediump float D = 0.20f;
const mediump float E = 0.02f;
const mediump float F = 0.30f;
const mediump vec3 W = vec3(11.2f, 11.2f, 11.2f);

mediump vec3 Uncharted2Tonemap(in mediump vec3 x)
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
	mediump vec3 mainColor = texture(u_MainTex, v_TexCoord).rgb;
	mediump vec3 bloomColor = texture(u_BloomTex, v_TexCoord).rgb;

	bloomColor *= k_bloomIntensity;
	mainColor += bloomColor;

	// mainColor *= 1.0f;
	mainColor = Uncharted2Tonemap(2.0f * mainColor);
	
	mediump vec3 whiteScale = 1.0f / Uncharted2Tonemap(W);
	mediump vec3 color = mainColor * whiteScale;
	color = pow(color, vec3(1.0f / 2.2f));

	o_Color = color;
}
