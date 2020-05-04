#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Scene
{
	mediump vec3 iu_CameraPos;
	highp mat4 iu_viewProjectionMatrix;
	mediump vec3 iu_sh[9];
};

layout(std140) uniform ub_Light
{
	mediump vec3 iu_LightDirection; // points to the light
	mediump vec3 iu_LightIntensity;
};

uniform mediump sampler2D u_AlbedoTex;
uniform mediump sampler2D u_MetalRoughnessTex;
uniform mediump sampler2D u_EmissionTex;
uniform mediump sampler2D u_SplitSum;
uniform mediump samplerCube u_PMREM;

in highp vec3 v_Normal_W;
in mediump vec3 v_ViewDir_W;
in mediump vec2 v_TexCoord;

mediump vec3 eval_sh_irradiance(in mediump vec3 i_normal)
{
	const mediump float c0 = 0.2820947918f;		// sqrt(1.0 / (4.0 * pi))
	const mediump float c1 = 0.4886025119f;		// sqrt(3.0 / (4.0 * pi))
	const mediump float c2 = 1.092548431f;		// sqrt(15.0 / (4.0 * pi))
	const mediump float c3 = 0.3153915653f;		// sqrt(5.0 / (16.0 * pi))
	const mediump float c4 = 0.5462742153f;		// sqrt(15.0 / (16.0 * pi))

	const mediump float a0 = 3.141593f;
	const mediump float a1 = 2.094395f;
	const mediump float a2 = 0.785398f;

	//TODO: we can pre-multiply those above factors with SH coeffs before transfering them
	//to the GPU

	return
		a0 * c0 * iu_sh[0]

		- a1 * c1 * i_normal.x * iu_sh[1]
		+ a1 * c1 * i_normal.y * iu_sh[2]
		- a1 * c1 * i_normal.z * iu_sh[3]

		+ a2 * c2 * i_normal.z * i_normal.x * iu_sh[4]
		- a2 * c2 * i_normal.x * i_normal.y * iu_sh[5]
		+ a2 * c3 * (3.0 * i_normal.y * i_normal.y - 1.0) * iu_sh[6]
		- a2 * c2 * i_normal.y * i_normal.z * iu_sh[7]
		+ a2 * c4 * (i_normal.z * i_normal.z - i_normal.x * i_normal.x) * iu_sh[8];
}

// fresnel
mediump vec3 fresnel_schlick_v3(in mediump vec3 v, in mediump vec3 n, in mediump vec3 f0)
{
	mediump float oneMinusVoN = max(1.0f - dot(v, n), 0.0f);
	return (f0 + (1.0f - f0) * pow(oneMinusVoN, 5.0f));
}

mediump float fresnel_spherical_gaussian(in mediump vec3 v, in mediump vec3 n, in mediump float f0)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (1.0f - f0) * exp2((-5.55473f * VoN - 6.98316f) * VoN));
}

mediump vec3 fresnel_spherical_gaussian_roughness(in mediump vec3 v, in mediump vec3 n, in mediump vec3 f0, in mediump float roughness)
{
	mediump float VoN = max(dot(v, n), 0.0f);
	return (f0 + (max(vec3(1.0f - roughness), f0) - f0) * exp2((-5.55473f * VoN - 6.98316f) * VoN));
}

// visibility
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
	mediump vec3 l = normalize(iu_LightDirection);
	mediump vec3 v = normalize(v_ViewDir_W);		// points to the camera
	mediump vec3 h = normalize(l + v);
	mediump vec3 n = normalize(v_Normal_W);
	
	mediump vec3 r = reflect(-v, n);

	mediump vec3 baseColor = pow(texture(u_AlbedoTex, v_TexCoord).rgb, vec3(2.2f));
	mediump vec2 metalRoughness = texture(u_MetalRoughnessTex, v_TexCoord).bg;
	mediump float occlusionFactor = texture(u_MetalRoughnessTex, v_TexCoord).r;
	mediump vec3 f0 = vec3(0.04f);
	f0 = mix(f0, baseColor, metalRoughness.x);

	// IBL
	mediump vec3 F_ibl = fresnel_spherical_gaussian_roughness(v, n, f0, metalRoughness.y);
	mediump vec3 kS_ibl = F_ibl;
	mediump vec3 kD_ibl = vec3(1.0f) - kS_ibl;
	kD_ibl *= 1.0 - metalRoughness.x;

	mediump vec3 irradiance = eval_sh_irradiance(n);
	mediump vec3 diffuseIBL = irradiance * baseColor;
	
	mediump vec3 prefilteredEnvColor = textureLod(u_PMREM, r, metalRoughness.y * 8.0f).rgb;
	mediump float nov = min(max(dot(n, v), 0.0f), 1.0);
	mediump vec2 precomputedEnvBRDF = texture(u_SplitSum, vec2(nov, 1.0f - metalRoughness.y)).rg;
	mediump vec3 specularIBL = prefilteredEnvColor * (F_ibl * precomputedEnvBRDF.x + precomputedEnvBRDF.y);
	
	mediump vec3 ambientIBL = kD_ibl * diffuseIBL + specularIBL;

	// PBR
	mediump float alpha = metalRoughness.y * metalRoughness.y;
	mediump float LoN = max(dot(l, n), 0.0f);
	mediump float VoN = max(dot(v, n), 0.0f);

	mediump vec3 fresnel = fresnel_schlick_v3(v, h, f0);
	mediump float ndf = ndf_ggx(n, h, metalRoughness.y);
	mediump float visibility = visibility_ggx_epicgames(l, v, n, h, metalRoughness.y);
	mediump vec3 brdf = (fresnel * ndf * visibility) / max(4.0f * LoN * VoN, 0.001f);

	mediump vec3 ks = fresnel;
	mediump vec3 kd = vec3(1.0f) - ks;
	kd *= (1.0f - metalRoughness.x);
	mediump vec3 Lo = (kd * baseColor / 3.14159f + brdf) * iu_LightIntensity.xyz * LoN;
	
	mediump vec3 emissionColor = texture(u_EmissionTex, v_TexCoord).rgb;
	Lo += emissionColor * 2.0f;
	
	Lo += ambientIBL;
	Lo *= occlusionFactor;
	Lo = baseColor;

	o_Color = vec4(Lo, 1.0f);
}