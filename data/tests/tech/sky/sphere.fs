#version 300 es
precision highp float;
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_Color;
in highp vec3 v_Normal;
in highp vec3 v_Position;

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

uniform mediump sampler2D u_TransmittanceTex;
uniform sampler2D u_IrradianceTex;

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

void main()
{
	mediump vec3 sky_irradiance;
	mediump vec3 sun_irradiance = GetSunAndSkyIrradiance(v_Position - configs_earthCenter,
		v_Normal, configs_sunDirection, sky_irradiance);
	mediump vec3 totalIrradiance = (sun_irradiance + sky_irradiance);
	o_Color = vec4(totalIrradiance, 1.0f);
}