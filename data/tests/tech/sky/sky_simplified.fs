#version 300 es
#define PI										3.14159265f
precision highp float;

layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 model_from_view;
	highp mat4 view_from_clip;
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
	highp vec3 configs_camera;
	mediump vec3 configs_whitePoint;
	highp vec3 configs_earthCenter;
	mediump vec3 configs_sunDirection;
	mediump vec2 configs_sunSize;
	mediump float configs_exposure;
	mediump float configs_lengthUnitInMeters;
};

uniform mediump sampler2D u_TransmittanceTex;
uniform mediump sampler2D u_IrradianceTex;
uniform mediump sampler3D u_ScatteringTex;

in mediump vec2 v_TexCoord;
in highp vec3 view_ray;

float SafeSqrt(float a)
{
	return sqrt(max(a, 0.0));
}

float ClampDistance(float d)
{
	return max(d, 0.0);
}

bool RayIntersectsGround(float r, float mu)
{
	return mu < 0.0 && r * r * (mu * mu - 1.0) + atmosphere_bottomRadius * atmosphere_bottomRadius >= 0.0;
}

float GetTextureCoordFromUnitRange(float x, int texture_size)
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

vec3 GetTransmittanceToTopAtmosphereBoundary(float r, float mu)
{
	vec2 uv = GetTransmittanceTextureUvFromRMu(r, mu);
	return texture(u_TransmittanceTex, uv).rgb;
}

vec4 GetScatteringTextureUvwzFromRMuMuSNu(float r, float mu, float mu_s, float nu, bool ray_r_mu_intersects_ground)
{
	float H = sqrt(atmosphere_topRadius * atmosphere_topRadius - atmosphere_bottomRadius * atmosphere_bottomRadius);
	float rho = SafeSqrt(r * r - atmosphere_bottomRadius * atmosphere_bottomRadius);
	float u_r = GetTextureCoordFromUnitRange(rho / H, textureInfo_scatteringTextureRSize);
	float r_mu = r * mu;
	float discriminant = r_mu * r_mu - r * r + atmosphere_bottomRadius * atmosphere_bottomRadius;
	float u_mu;

	if (ray_r_mu_intersects_ground)
	{
		float d = -r_mu - SafeSqrt(discriminant);
		float d_min = r - atmosphere_bottomRadius;
		float d_max = rho;
		u_mu = 0.5 - 0.5 * GetTextureCoordFromUnitRange(d_max == d_min ? 0.0 :
				(d - d_min) / (d_max - d_min), textureInfo_scatteringTextureMuSize / 2);
	}
	else
	{
		float d = -r_mu + SafeSqrt(discriminant + H * H);
		float d_min = atmosphere_topRadius - r;
		float d_max = rho + H;
		u_mu = 0.5 + 0.5 * GetTextureCoordFromUnitRange(
				(d - d_min) / (d_max - d_min), textureInfo_scatteringTextureMuSize / 2);
	}

	float d = DistanceToTopAtmosphereBoundary(atmosphere_bottomRadius, mu_s);
	float d_min = atmosphere_topRadius - atmosphere_bottomRadius;
	float d_max = H;

	float a = (d - d_min) / (d_max - d_min);
	float A = -2.0 * atmosphere_muSMin * atmosphere_bottomRadius / (d_max - d_min);
	float u_mu_s = GetTextureCoordFromUnitRange(max(1.0 - a / A, 0.0) / (1.0 + a), textureInfo_scatteringTextureMuSSize);
	float u_nu = (nu + 1.0) / 2.0;
	return vec4(u_nu, u_mu_s, u_mu, u_r);
}

vec3 GetExtrapolatedSingleMieScattering(vec4 scattering)
{
	if (scattering.r <= 0.0)
	{
		return vec3(0.0);
	}
	return scattering.rgb * scattering.a / scattering.r *
		(atmosphere_rayleighScattering.r / atmosphere_mieScattering.r) *
		(atmosphere_mieScattering / atmosphere_rayleighScattering);
}

vec3 GetCombinedScattering(float r, float mu, float mu_s, float nu, bool ray_r_mu_intersects_ground,
		out vec3 single_mie_scattering)
{
	vec4 uvwz = GetScatteringTextureUvwzFromRMuMuSNu(r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	float tex_coord_x = uvwz.x * float(textureInfo_scatteringTextureNuSize - 1);
	float tex_x = floor(tex_coord_x);
	float lerp = tex_coord_x - tex_x;
	vec3 uvw0 = vec3((tex_x + uvwz.y) / float(textureInfo_scatteringTextureNuSize), uvwz.z, uvwz.w);
	vec3 uvw1 = vec3((tex_x + 1.0 + uvwz.y) / float(textureInfo_scatteringTextureNuSize), uvwz.z, uvwz.w);

	vec4 combined_scattering = texture(u_ScatteringTex, uvw0) * (1.0 - lerp) + texture(u_ScatteringTex, uvw1) * lerp;
	vec3 scattering = combined_scattering.rgb;
	single_mie_scattering = GetExtrapolatedSingleMieScattering(combined_scattering);
	return scattering;
}

float RayleighPhaseFunction(float nu)
{
	float k = 3.0 / (16.0 * PI);
	return k * (1.0 + nu * nu);
}

float MiePhaseFunction(float g, float nu)
{
	float k = 3.0 / (8.0 * PI) * (1.0 - g * g) / (2.0 + g * g);
	return k * (1.0 + nu * nu) / pow(1.0 + g * g - 2.0 * g * nu, 1.5);
}

vec3 GetSkyRadiance(vec3 camera, vec3 view_ray, vec3 sun_direction, out vec3 transmittance)
{
	float r = length(camera);
	float rmu = dot(camera, view_ray);
	float distToTop = -rmu - sqrt(rmu * rmu - r * r + atmosphere_topRadius * atmosphere_topRadius);

	if (distToTop > 0.0)
	{
		camera = camera + view_ray * distToTop;
		r = atmosphere_topRadius;
		rmu += distToTop;
	}
	else if (r > atmosphere_topRadius)
	{
		transmittance = vec3(1.0);
		return vec3(0.0);
	}

	float mu = rmu / r;
	float mu_s = dot(camera, sun_direction) / r;
	float nu = dot(view_ray, sun_direction);
	bool ray_r_mu_intersects_ground = RayIntersectsGround(r, mu);

	transmittance = ray_r_mu_intersects_ground ? vec3(0.0) : GetTransmittanceToTopAtmosphereBoundary(r, mu);

	vec3 single_mie_scattering;
	vec3 scattering;

	scattering = GetCombinedScattering(r, mu, mu_s, nu, ray_r_mu_intersects_ground, single_mie_scattering);
	return scattering * RayleighPhaseFunction(nu) +
		single_mie_scattering * MiePhaseFunction(atmosphere_miePhaseFunctionG, nu);
}

vec3 GetSolarRadiance()
{
	return atmosphere_solarIrradiance / (PI * atmosphere_sunAngularRadius * atmosphere_sunAngularRadius);
}

void main()
{
	vec3 view_direction = normalize(view_ray);
	vec3 transmittance;
	vec3 radiance = GetSkyRadiance(configs_camera - configs_earthCenter, view_direction, configs_sunDirection, transmittance);

	if (dot(view_direction, configs_sunDirection) > configs_sunSize.y)
	{
		radiance = radiance + transmittance * GetSolarRadiance();
	}

	o_Color.rgb = pow(vec3(1.0) - exp(-radiance / configs_whitePoint * configs_exposure), vec3(1.0 / 2.2));
	o_Color.a = 1.0;
}
