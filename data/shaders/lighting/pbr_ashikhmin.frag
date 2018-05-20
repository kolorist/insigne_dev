#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec3 v_Normal_W;
in mediump vec3 v_Tangent_W;
in mediump vec3 v_Bitangent_W;
in mediump vec3 v_ViewDir_W;

uniform mediump vec3 iu_LightDirection;
uniform mediump vec3 iu_Ka;

// fresnel
mediump float fresnel_schlick(in mediump vec3 v, in mediump vec3 n, in mediump float f0)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (1.0f - f0) * pow(1.0f - VoN, 5.0f));
}

// ashikhmin specular component
mediump float ashikhmin_ps(in mediump vec3 l, in mediump vec3 v, in mediump vec3 n, in mediump vec3 u,
	in mediump vec3 v, in mediump vec3 h,
	in mediump float nu, in mediump float  nv)
{
	mediump float NoH = max(dot(n, h), 0.0f);
	mediump float HoV = max(dot(h, v), 0.0f);
	mediump float NoL = max(dot(n, l), 0.0f);

	mediump float ps1 = sqrt((nu + 1.0f) * (nv + 1.0f)) / (8.0f * 3.14159f);
	mediump float ps2_exp = (nu * dot(h, u) * dot(h, u) + nv * dot(h, v) * dot(h, v)) / (1.0f - dot(h, n) * dot(h, n));
	mediump float ps2_denom = HoV * max(NoL, NoV);
	mediump float ps2 = NoH * ps2_exp / ps2_denom;
	mediump float psFresnel = fresnel_schlick(v, h);

	return ps1 * ps2 * psFresnel;
}

// ashikhmin diffuse component
mediump float ashikhmin_pd(in mediump vec3 v, in mediump vec3 l, in mediump vec3 n, in mediump vec3 rs,
	in mediump vec3 rd)
{
	mediump float NoL = max(dot(n, l), 0.0f);
	mediump float NoV = max(dot(n, v), 0.0f);

	mediump float pd1 = 28.0f * rd * (1.0f - rs) / (23.0f * 3.14159f);
	mediump float pd2 = 1.0f - pow(1.0f - NoV / 2.0f, 5.0f);
	mediump float pd3 = 1.0f - pow(1.0f - NoL / 2.0f, 5.0f);

	return pd1 * pd2 * pd3;
}

void main()
{
	mediump vec3 l = normalize(iu_LightDirection);
	mediump vec3 v = normalize(v_ViewDir_W);	// does normalizing need to be done?
	mediump vec3 h = normalize(l + v);
	mediump vec3 n = normalize(v_Normal_W);
	mediump vec3 u = normalize(v_Tangent_W);
	mediump vec3 v = normalize(v_Bitangent_W);

	mediump float nu = 400.0f;
	mediump float nv = 400.0f;
	mediump float rs = iu_Ka;
	mediump float rd = iu_Ka;

	mediump float brdf_ps = ashikhmin_ps(l, v, n, u, v, h, nu, nv);
	mediump float brdf_pd = ashikhmin_pd(v, l, n, rs, rd);

	o_Color = vec4(vec3(brdf_ps + brdf_pd), 1.0f);
}
