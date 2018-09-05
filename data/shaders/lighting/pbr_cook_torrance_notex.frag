#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec3 v_Normal_W;
in mediump vec3 v_ViewDir_W;
in mediump vec2 v_TexCoord;

uniform mediump vec3 iu_LightDirection;
uniform mediump vec3 iu_LightIntensity;
uniform mediump vec3 iu_Ka;

uniform mediump vec3 iu_BaseColor;
uniform mediump float iu_Metallic;
uniform mediump float iu_Roughness;

uniform samplerCube iu_IrrMap;
uniform samplerCube iu_SpecMap;
uniform sampler2D iu_SplitSumLUT;

// fresnel
mediump float fresnel_schlick(in mediump vec3 v, in mediump vec3 n, in mediump float f0)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (1.0f - f0) * pow(1.0f - VoN, 5.0f));
}

mediump vec3 fresnel_schlick_v3(in mediump vec3 v, in mediump vec3 n, in mediump vec3 f0)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (1.0f - f0) * pow(1.0f - VoN, 5.0f));
}

// for ibl
mediump vec3 fresnel_schlick_v3_roughness(in mediump vec3 v, in mediump vec3 n, in mediump vec3 f0, in mediump float roughness)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (max(vec3(1.0f - roughness), f0) - f0) * pow(1.0f - VoN, 5.0f));
}

mediump float fresnel_spherical_gaussian(in mediump vec3 v, in mediump vec3 n, in mediump float f0)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (1.0f - f0) * exp2((-5.55473f * VoN - 6.98316f) * VoN));
}

// visibility
mediump float visibility_implicit(in mediump vec3 l, in mediump vec3 v, in mediump vec3 n,
	in mediump vec3 h, in mediump float roughness)
{
	mediump float LoN = max(dot(l, n), 0.0f);
	mediump float VoN = max(dot(v, n), 0.0f);

	return (LoN * VoN);
}

mediump float visibility_torrance_sparrow(in mediump vec3 l, in mediump vec3 v, in mediump vec3 n,
	in mediump vec3 h, in mediump float roughness)
{
	mediump float HoN = max(dot(h, n), 0.0f);
	mediump float VoN = max(dot(v, n), 0.0f);
	mediump float VoH = max(dot(v, h), 0.0f);	// equals to LoH
	mediump float LoN = max(dot(l, n), 0.0f);
	mediump float f1 = 2.0f * HoN * VoN / VoH;
	mediump float f2 = 2.0f * HoN * LoN / VoH;
	return min(1.0f, min(f1, f2));
}

mediump float visibility_ggx_epicgames(in mediump vec3 l, in mediump vec3 v, in mediump vec3 n,
	in mediump vec3 h, in mediump float roughness)
{
	mediump float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
	mediump float VoN = max(dot(v, n), 0.0f);
	mediump float LoN = max(dot(l, n), 0.0f);
	mediump float ggxL = LoN / (LoN * (1.0f - k) + k);
	mediump float ggxV = VoN / (VoN * (1.0f - k) + k);
	return (ggxL * ggxV);
}

// normal distribution function
mediump float ndf_ggx(in mediump vec3 n, in mediump vec3 h, in mediump float roughness)
{
	mediump float alpha = roughness * roughness;
	mediump float NoH = max(dot(n, h), 0.0f);
	mediump float alphaSq = alpha * alpha;
	return (alphaSq /
		(3.14159f * pow(1.0f + NoH * NoH * (alphaSq - 1.0f), 2.0f)));
}

// BRDFs
mediump float brdf_cook_torrance(in mediump vec3 l, in mediump vec3 v, in mediump vec3 h, in mediump vec3 n,
	in mediump float roughness, in mediump float f0)
{
	mediump float alpha = roughness * roughness;
	mediump float LoN = max(dot(l, n), 0.0f);
	mediump float VoN = max(dot(v, n), 0.0f);

	mediump float fresnel = fresnel_spherical_gaussian(v, n, f0);
	mediump float ndf = ndf_ggx(n, h, alpha);
	mediump float visibility = visibility_ggx_epicgames(l, v, n, h, roughness);

	return (fresnel * ndf * visibility) / max(4.0f * LoN * VoN, 0.001f);
}

void main()
{
	mediump vec3 l = normalize(iu_LightDirection);	// points to the light source
	mediump vec3 v = normalize(v_ViewDir_W);		// points to the camera
	mediump vec3 h = normalize(l + v);
	mediump vec3 n = normalize(v_Normal_W);\
	mediump vec3 r = reflect(-v, n);

	mediump float roughness = iu_Roughness;
	mediump float metallic = iu_Metallic;
	mediump vec3 baseColor = iu_BaseColor;

	mediump vec3 f0 = vec3(0.04f);
	f0 = mix(f0, baseColor, metallic);

	// PBR
	mediump float alpha = roughness * roughness;
	mediump float LoN = max(dot(l, n), 0.0f);
	mediump float VoN = max(dot(v, n), 0.0f);

	mediump vec3 fresnel = fresnel_schlick_v3(v, h, f0);
	mediump float ndf = ndf_ggx(n, h, roughness);
	mediump float visibility = visibility_ggx_epicgames(l, v, n, h, roughness);
	mediump vec3 brdf = (fresnel * ndf * visibility) / max(4.0f * LoN * VoN, 0.001f);

	mediump vec3 ks = fresnel;
	mediump vec3 kd = vec3(1.0f) - ks;
	kd *= (1.0f - metallic);
	mediump vec3 Lo = (kd * baseColor / 3.14159f + brdf) * iu_LightIntensity * LoN;

	// IBL
	mediump vec3 F_ibl = fresnel_schlick_v3_roughness(v, n, f0, roughness);
	//mediump vec3 F_ibl = fresnel_schlick_v3(v, n, f0);
	mediump vec3 kS_ibl = F_ibl;
	mediump vec3 kD_ibl = vec3(1.0f) - kS_ibl;
	kD_ibl *= 1.0 - metallic;
	
	mediump vec3 irradiance = texture(iu_IrrMap, n).rgb;
	mediump vec3 diffuseIBL = irradiance * baseColor;
	
	mediump vec3 prefilteredEnvColor = textureLod(iu_SpecMap, r, roughness * 9.0f).rgb;
	mediump float nov = min(max(dot(n, v), 0.0f), 0.99);
	mediump vec2 precomputedEnvBRDF = texture(iu_SplitSumLUT, vec2(nov, 1.0f - roughness)).rg;
	mediump vec3 specularIBL = prefilteredEnvColor * (F_ibl * precomputedEnvBRDF.x + precomputedEnvBRDF.y);
	
	mediump vec3 ambientIBL = kD_ibl * diffuseIBL + specularIBL;

	o_Color = vec4(Lo + ambientIBL, 1.0f);
}
