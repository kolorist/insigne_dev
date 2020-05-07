#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
	mediump vec3 iu_cameraPos;
	mediump vec3 iu_sh[9];
};

layout(std140) uniform ub_Lighting
{
	mediump vec3 iu_LightDirection; // points to the light
	mediump vec3 iu_LightIntensity;
};

uniform mediump sampler2D u_AlbedoTex;
uniform mediump sampler2D u_NormalTex;
uniform mediump sampler2D u_AttribTex;
uniform mediump samplerCube u_PMREMTex;
uniform mediump sampler2D u_SplitSumTex;

in mediump mat3 v_TBN;
in mediump vec2 v_TexCoord;
in mediump vec3 v_ViewDir_W;

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

mediump vec3 decode_rgbm(in mediump vec4 i_rgbmColor)
{
	mediump vec3 hdrColor = 6.0f * i_rgbmColor.rgb * i_rgbmColor.a;
	return (hdrColor * hdrColor);
}

mediump vec3 srgb_to_linear(in mediump vec3 i_srgbColor)
{
	return pow(i_srgbColor, vec3(2.2f));
}

void main()
{
	// base vectors
	mediump vec3 l = normalize(iu_LightDirection);
	mediump vec3 v = normalize(v_ViewDir_W);		// points to the camera
	mediump vec3 h = normalize(l + v);
	mediump vec3 n = texture(u_NormalTex, v_TexCoord).rgb;
	n = normalize(n * 2.0f - 1.0f);
	n = normalize(v_TBN * n);
	mediump vec3 r = normalize(reflect(-v, n));

	// angular terms
	mediump float nov = min(max(dot(n, v), 0.0f), 1.0);
	mediump float voh = min(max(dot(v, h), 0.0f), 1.0f);
	mediump float nol = min(max(dot(n, l), 0.0f), 1.0f);
	mediump float noh = min(max(dot(n, h), 0.0f), 1.0f);

	// textures
	mediump vec4 baseColor = texture(u_AlbedoTex, v_TexCoord);
	mediump vec2 metalRoughness = texture(u_AttribTex, v_TexCoord).bg;
	mediump float roughness = clamp(metalRoughness.y, 0.0f, 1.0f);
	mediump float metallic = clamp(metalRoughness.x, 0.0f, 1.0f);

	// The following implementation is according to https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#appendix-b-brdf-implementation
	const mediump vec3 dielectricSpecular = vec3(0.04f);
	// black is vec3(0.0f, 0.0f, 0.0f)
	mediump vec3 diffuseColor = baseColor.rgb * (vec3(1.0f) - dielectricSpecular) * (1.0f - metallic);
	mediump vec3 F0 = mix(dielectricSpecular, baseColor.rgb, metallic); // reflectance color at normal incidence
	mediump float alpha = roughness * roughness;

	// -begin- ibl
	mediump vec3 irradiance = eval_sh_irradiance(n);
	mediump vec3 diffuseIBL = irradiance * diffuseColor;

	mediump vec3 prefilteredEnvColor = decode_rgbm(textureLod(u_PMREMTex, r, roughness * 8.0f));
	mediump vec2 precomputedBRDF = texture(u_SplitSumTex, vec2(nov, roughness)).rg;
	mediump vec3 specularIBL = prefilteredEnvColor * (F0 * precomputedBRDF.x + precomputedBRDF.y);

	mediump vec3 iblContrib = diffuseIBL + specularIBL;
	// -end- ibl

	// -begin- direct lighting
	mediump vec3 brdfMulCosineTerm = vec3(0.0f);
	if (nol > 0.0f || nov > 0.0f)
	{
		// surface reflection ratio (F)
		#if 0
			// simplified implementation of schlick's approximation
			// equation 9.16 - Real-Time Rendering 4th edition - page 320
			// see https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#surface-reflection-ratio-f
			mediump vec3 F = F0 + (vec3(1.0f) - F0) * pow(clamp(1.0f - voh, 0.0f, 1.0f), 5.0f);
		#else
			// more general form of the schlick's approximation
			// equation 9.18 - Real-Time Rendering 4th edition - page 321
			// see https://github.com/KhronosGroup/glTF-Sample-Viewer#surface-reflection-ratio-f
			mediump float maxReflectance = max(max(F0.r, F0.g), F0.b);
			mediump vec3 F90 = vec3(clamp(maxReflectance * 50.0f, 0.0f, 1.0f));
			mediump vec3 F = F0 + (F90 - F0) * pow(clamp(1.0f - voh, 0.0f, 1.0f), 5.0f);
		#endif

		// geometric occlusion (G)
		// Smith Joint GGX
		// Note: Vis = G / (4 * NdotL * NdotV)
		// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
		// see Real-Time Rendering. Page 331 to 336.
		// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
		// see https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#geometric-occlusion-g
		mediump float alphaSq = alpha * alpha;
		mediump float GGXV = nol * sqrt(nov * nov * (1.0f - alphaSq) + alphaSq);
		mediump float GGXL = nov * sqrt(nol * nol * (1.0f - alphaSq) + alphaSq);
		mediump float GGX = GGXV + GGXL;
		
		mediump float vis = 0.0f;
		if (GGX > 0.0f)
		{
			vis = 0.5f / GGX;
		}

		// microfacet distribution (D)
		// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
		// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
		// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
		// see https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#microfacet-distribution-d
		mediump float tmp = (noh * alphaSq - noh) * noh + 1.0f;
		mediump float D = alphaSq / (3.141593f * tmp * tmp);

		mediump vec3 diffuseContrib = (1.0f - F) * diffuseColor / 3.141593f;
		mediump vec3 specContrib = F * vis * D;

		brdfMulCosineTerm = nol * (diffuseContrib + specContrib);
	}
	mediump vec3 directContrib = iu_LightIntensity * brdfMulCosineTerm;
	// -end- direct lighting

	mediump vec3 radiance = iblContrib + directContrib;

	o_Color = vec4(radiance, baseColor.a);
}