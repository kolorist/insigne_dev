#version 300 es
precision highp float;
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_viewProjectionMatrix;
	highp mat4 iu_xformMatrix;
	mediump vec3 iu_CameraPos;
};

layout(std140) uniform ub_Light
{
	mediump vec3 iu_LightDirection; // points to the light
	mediump vec3 iu_LightIntensity;
};

layout(std140) uniform ub_TextureInfo
{
	mediump int textureInfo_transmittanceTextureWidth;
	mediump int textureInfo_transmittanceTextureHeight;

	mediump int textureInfo_scatteringTextureRSize;
	mediump int textureInfo_scatteringTextureMuSize;
	mediump int textureInfo_scatteringTextureMuSSize;
	mediump int textureInfo_scatteringTextureNuSize;

	mediump int textureInfo_scatteringTextureWidth;
	mediump int textureInfo_scatteringTextureHeight;
	mediump int textureInfo_scatteringTextureDepth;

	mediump int textureInfo_irrandianceTextureWidth;
	mediump int textureInfo_irrandianceTextureHeight;
};

layout(std140) uniform ub_Atmosphere
{
	mediump vec3 atmosphere_solarIrradiance;
	mediump vec3 atmosphere_rayleighScattering;
	mediump vec3 atmosphere_mieScattering;
	mediump float atmosphere_sunAngularRadius;
	mediump float atmosphere_bottomRadius;
	mediump float atmosphere_topRadius;
	mediump float atmosphere_miePhaseFunctionG;
	mediump float atmosphere_muSMin;
};

layout(std140) uniform ub_Configs
{
	mediump vec3 configs_camera;
	mediump vec3 configs_whitePoint;
	mediump vec3 configs_earthCenter;
	mediump vec3 configs_sunDirection;
	mediump vec2 configs_sunSize;
	mediump float configs_exposure;
	mediump float configs_lengthUnitInMeters;
};

uniform mediump sampler2D u_AlbedoTex;
uniform mediump sampler2D u_MetalRoughnessTex;
uniform mediump sampler2D u_NormalTex;
uniform mediump sampler2D u_EmissionTex;
uniform mediump sampler2D u_SplitSumTex;
uniform mediump sampler2D u_TransmittanceTex;
uniform mediump sampler2D u_IrradianceTex;
uniform mediump samplerCube u_PMREMTex;

in mediump mat3 v_TBN;
in mediump vec3 v_ViewDir_W;
in mediump vec2 v_TexCoord;
in highp vec3 v_Position;

float SafeSqrt(float a)
{
	return sqrt(max(a, 0.0));
}

float ClampDistance(float d)
{
	return max(d, 0.0);
}

mediump float GetTextureCoordFromUnitRange(highp float x, highp int texture_size)
{
	return 0.5 / float(texture_size) + x * (1.0 - 1.0 / float(texture_size));
}

float DistanceToTopAtmosphereBoundary(float r, float mu)
{
	float discriminant = r * r * (mu * mu - 1.0) + atmosphere_topRadius * atmosphere_topRadius;
	return ClampDistance(-r * mu + SafeSqrt(discriminant));
}

vec2 GetTransmittanceTextureUvFromRMu(float r, float mu)
{
	float H = sqrt(atmosphere_topRadius * atmosphere_topRadius - atmosphere_bottomRadius * atmosphere_bottomRadius);
	float rho = SafeSqrt(r * r - atmosphere_bottomRadius * atmosphere_bottomRadius);
	float d = DistanceToTopAtmosphereBoundary(r, mu);
	float d_min = atmosphere_topRadius - r;
	float d_max = rho + H;
	float x_mu = (d - d_min) / (d_max - d_min);
	float x_r = rho / H;
	return vec2(GetTextureCoordFromUnitRange(x_mu, textureInfo_transmittanceTextureWidth),
			GetTextureCoordFromUnitRange(x_r, textureInfo_transmittanceTextureHeight));
}

mediump vec3 GetTransmittanceToTopAtmosphereBoundary(float r, float mu)
{
	vec2 uv = GetTransmittanceTextureUvFromRMu(r, mu);
	return texture(u_TransmittanceTex, uv).rgb;
}

mediump vec2 GetIrradianceTextureUvFromRMuS(highp float r, highp float mu_s)
{
	highp float x_r = (r - atmosphere_bottomRadius) / (atmosphere_topRadius - atmosphere_bottomRadius);
	highp float x_mu_s = mu_s * 0.5 + 0.5;
	return vec2(GetTextureCoordFromUnitRange(x_mu_s, textureInfo_irrandianceTextureWidth),
			  GetTextureCoordFromUnitRange(x_r, textureInfo_irrandianceTextureHeight));
}

mediump vec3 GetIrradiance(highp float r, highp float mu_s)
{
	mediump vec2 uv = GetIrradianceTextureUvFromRMuS(r, mu_s);
	return texture(u_IrradianceTex, uv).rgb;
}

mediump vec3 GetTransmittanceToSun(highp float r, highp float mu_s)
{
  mediump float sin_theta_h = atmosphere_bottomRadius / r;
  mediump float cos_theta_h = -sqrt(max(1.0 - sin_theta_h * sin_theta_h, 0.0));
  return GetTransmittanceToTopAtmosphereBoundary(r, mu_s) *
      smoothstep(-sin_theta_h * atmosphere_sunAngularRadius,
                 sin_theta_h * atmosphere_sunAngularRadius,
                 mu_s - cos_theta_h);
}

mediump vec3 GetSunAndSkyIrradiance(in highp vec3 point, in highp vec3 normal, in mediump vec3 sun_direction,
    out mediump vec3 sky_irradiance)
{
	highp float r = length(point);
	highp float mu_s = dot(point, sun_direction) / r;
	sky_irradiance = GetIrradiance(r, mu_s) * (1.0 + dot(normal, point) / r) * 0.5;
	return atmosphere_solarIrradiance * GetTransmittanceToSun(r, mu_s) * max(dot(normal, sun_direction), 0.0);
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

mediump vec3 get_safe_hdr(in mediump vec3 i_hdrColor)
{
	return clamp(i_hdrColor, vec3(0.0f, 0.0f, 0.0f), vec3(36.0f, 36.0f, 36.0f));
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
	mediump vec3 baseColor = texture(u_AlbedoTex, v_TexCoord).rgb;
	mediump vec3 emissionColor = texture(u_EmissionTex, v_TexCoord).rgb;
	mediump vec3 occlusionRounghessMetal = texture(u_MetalRoughnessTex, v_TexCoord).rgb;
	mediump float occlusion = clamp(occlusionRounghessMetal.r, 0.0f, 1.0f);
	mediump float roughness = clamp(occlusionRounghessMetal.g, 0.0f, 1.0f);
	mediump float metallic = clamp(occlusionRounghessMetal.b, 0.0f, 1.0f);

	// The following implementation is according to https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md#appendix-b-brdf-implementation
	const mediump vec3 dielectricSpecular = vec3(0.04f);
	// black is vec3(0.0f, 0.0f, 0.0f)
	mediump vec3 diffuseColor = baseColor * (vec3(1.0f) - dielectricSpecular) * (1.0f - metallic);
	mediump vec3 F0 = mix(dielectricSpecular, baseColor, metallic); // reflectance color at normal incidence
	mediump float alpha = roughness * roughness;

	// -begin- ibl
	mediump vec3 sky_irradiance;
	mediump vec3 sun_irradiance = GetSunAndSkyIrradiance(v_Position, n, configs_sunDirection, sky_irradiance);
	mediump vec3 irradiance = sun_irradiance + sky_irradiance;
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
	radiance += emissionColor;
	radiance *= occlusion;

	o_Color = vec4(get_safe_hdr(radiance), 1.0f);
}