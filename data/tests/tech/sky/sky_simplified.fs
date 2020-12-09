#version 300 es
precision highp float;

layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 model_from_view;
	highp mat4 view_from_clip;
};

uniform mediump sampler2D u_TransmittanceTex;
uniform mediump sampler2D u_IrradianceTex;
uniform mediump sampler3D u_ScatteringTex;

in mediump vec2 v_TexCoord;
in highp vec3 view_ray;

const int TRANSMITTANCE_TEXTURE_WIDTH = 256;
const int TRANSMITTANCE_TEXTURE_HEIGHT = 64;
const int SCATTERING_TEXTURE_R_SIZE = 32;
const int SCATTERING_TEXTURE_MU_SIZE = 128;
const int SCATTERING_TEXTURE_MU_S_SIZE = 32;
const int SCATTERING_TEXTURE_NU_SIZE = 8;
const int IRRADIANCE_TEXTURE_WIDTH = 64;
const int IRRADIANCE_TEXTURE_HEIGHT = 16;

struct DensityProfileLayer
{
	float width;
	float exp_term;
	float exp_scale;
	float linear_term;
	float constant_term;
};

struct DensityProfile
{
	DensityProfileLayer layers[2];
};

struct AtmosphereParameters
{
	vec3 solar_irradiance;
	float sun_angular_radius;
	float bottom_radius;
	float top_radius;
	DensityProfile rayleigh_density;
	vec3 rayleigh_scattering;
	DensityProfile mie_density;
	vec3 mie_scattering;
	vec3 mie_extinction;
	float mie_phase_function_g;
	DensityProfile absorption_density;
	vec3 absorption_extinction;
	vec3 ground_albedo;
	float mu_s_min;
};

const AtmosphereParameters ATMOSPHERE = AtmosphereParameters(
		vec3(1.474000,1.850400,1.911980),
		0.004675,
		6360.000000,
		6420.000000,
		DensityProfile(DensityProfileLayer[2](DensityProfileLayer(0.000000,0.000000,0.000000,0.000000,0.000000),DensityProfileLayer(0.000000,1.000000,-0.125000,0.000000,0.000000))),
		vec3(0.005802,0.013558,0.033100),
		DensityProfile(DensityProfileLayer[2](DensityProfileLayer(0.000000,0.000000,0.000000,0.000000,0.000000),DensityProfileLayer(0.000000,1.000000,-0.833333,0.000000,0.000000))),
		vec3(0.003996,0.003996,0.003996),
		vec3(0.004440,0.004440,0.004440),
		0.800000,
		DensityProfile(DensityProfileLayer[2](DensityProfileLayer(25.000000,0.000000,0.000000,0.066667,-0.666667),DensityProfileLayer(0.000000,0.000000,0.000000,-0.066667,2.666667))),
		vec3(0.000650,0.001881,0.000085),
		vec3(0.100000,0.100000,0.100000),
		-0.207912);

const vec3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(114974.916437,71305.954816,65310.548555);
const vec3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(98242.786222,69954.398112,66475.012354);

const float kLengthUnitInMeters = 1000.000000;
//const vec3 camera = vec3(8.90958500, -0.893940210, 0.905631602);
const vec3 camera = vec3(8.9543, 0, 0.9056);
const float exposure = 10.0;
const vec3 white_point = vec3(1.0, 1.0, 1.0);
const vec3 earth_center = vec3(0.0, 0.0, -6360.0f);
//const vec3 sun_direction = vec3(cos(-3.0) * sin(1.564), sin(-3.0) * sin(1.564), cos(1.564)); //1.564, -3.0
const vec3 sun_direction = vec3(-0.9900, -0.1411, 0.0068);
const vec2 sun_size = vec2(tan(0.004675), cos(0.004675));
const float PI = 3.14159265;
const vec3 kSphereCenter = vec3(0.0, 0.0, 1000.0) / kLengthUnitInMeters;
const float kSphereRadius = 1000.0 / kLengthUnitInMeters;
const vec3 kSphereAlbedo = vec3(0.8);
const vec3 kGroundAlbedo = vec3(0.0, 0.0, 0.04);

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
	return mu < 0.0 && r * r * (mu * mu - 1.0) + ATMOSPHERE.bottom_radius * ATMOSPHERE.bottom_radius >= 0.0;
}

float GetTextureCoordFromUnitRange(float x, int texture_size)
{
	return 0.5 / float(texture_size) + x * (1.0 - 1.0 / float(texture_size));
}

float DistanceToTopAtmosphereBoundary(float r, float mu)
{
	float discriminant = r * r * (mu * mu - 1.0) + ATMOSPHERE.top_radius * ATMOSPHERE.top_radius;
	return ClampDistance(-r * mu + SafeSqrt(discriminant));
}

vec2 GetTransmittanceTextureUvFromRMu(float r, float mu)
{
	// --
	return vec2(0.0);
	// --
	float H = sqrt(ATMOSPHERE.top_radius * ATMOSPHERE.top_radius - ATMOSPHERE.bottom_radius * ATMOSPHERE.bottom_radius);
	float rho = SafeSqrt(r * r - ATMOSPHERE.bottom_radius * ATMOSPHERE.bottom_radius);
	float d = DistanceToTopAtmosphereBoundary(r, mu);
	float d_min = ATMOSPHERE.top_radius - r;
	float d_max = rho + H;
	float x_mu = (d - d_min) / (d_max - d_min);
	float x_r = rho / H;
	return vec2(GetTextureCoordFromUnitRange(x_mu, TRANSMITTANCE_TEXTURE_WIDTH),
			GetTextureCoordFromUnitRange(x_r, TRANSMITTANCE_TEXTURE_HEIGHT));
}

vec3 GetTransmittanceToTopAtmosphereBoundary(float r, float mu)
{
	vec2 uv = GetTransmittanceTextureUvFromRMu(r, mu);
	return texture(u_TransmittanceTex, uv).rgb;
}

vec4 GetScatteringTextureUvwzFromRMuMuSNu(float r, float mu, float mu_s, float nu, bool ray_r_mu_intersects_ground)
{
	float H = sqrt(ATMOSPHERE.top_radius * ATMOSPHERE.top_radius - ATMOSPHERE.bottom_radius * ATMOSPHERE.bottom_radius);
	float rho = SafeSqrt(r * r - ATMOSPHERE.bottom_radius * ATMOSPHERE.bottom_radius);
	float u_r = GetTextureCoordFromUnitRange(rho / H, SCATTERING_TEXTURE_R_SIZE);
	float r_mu = r * mu;
	float discriminant = r_mu * r_mu - r * r + ATMOSPHERE.bottom_radius * ATMOSPHERE.bottom_radius;
	float u_mu;
	
	if (ray_r_mu_intersects_ground)
	{
		float d = -r_mu - SafeSqrt(discriminant);
		float d_min = r - ATMOSPHERE.bottom_radius;
		float d_max = rho;
		u_mu = 0.5 - 0.5 * GetTextureCoordFromUnitRange(d_max == d_min ? 0.0 :
				(d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
	}
	else
	{
		float d = -r_mu + SafeSqrt(discriminant + H * H);
		float d_min = ATMOSPHERE.top_radius - r;
		float d_max = rho + H;
		u_mu = 0.5 + 0.5 * GetTextureCoordFromUnitRange(
				(d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
	}

	float d = DistanceToTopAtmosphereBoundary(/*ATMOSPHERE.bottom_radius*/6360.0, mu_s);
	float d_min = 6420.0 - 6360.0 /*ATMOSPHERE.top_radius - ATMOSPHERE.bottom_radius*/;
	float d_max = H;

	float a = (d - d_min) / (d_max - d_min);
	float A = -2.0 * ATMOSPHERE.mu_s_min * ATMOSPHERE.bottom_radius / (d_max - d_min);
	float u_mu_s = GetTextureCoordFromUnitRange(max(1.0 - a / A, 0.0) / (1.0 + a), SCATTERING_TEXTURE_MU_S_SIZE);
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
		(ATMOSPHERE.rayleigh_scattering.r / ATMOSPHERE.mie_scattering.r) *
		(ATMOSPHERE.mie_scattering / ATMOSPHERE.rayleigh_scattering);
}

vec3 GetCombinedScattering(float r, float mu, float mu_s, float nu, bool ray_r_mu_intersects_ground,
		out vec3 single_mie_scattering)
{	
	vec4 uvwz = GetScatteringTextureUvwzFromRMuMuSNu(r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	float tex_coord_x = uvwz.x * float(SCATTERING_TEXTURE_NU_SIZE - 1);
	float tex_x = floor(tex_coord_x);
	float lerp = tex_coord_x - tex_x;
	vec3 uvw0 = vec3((tex_x + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE), uvwz.z, uvwz.w);
	vec3 uvw1 = vec3((tex_x + 1.0 + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE), uvwz.z, uvwz.w);

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
	float distToTop = -rmu - sqrt(rmu * rmu - r * r + ATMOSPHERE.top_radius * ATMOSPHERE.top_radius);

	if (distToTop > 0.0)
	{
		camera = camera + view_ray * distToTop;
		r = 6420.0 /*ATMOSPHERE.top_radius*/;
		rmu += distToTop;
	}
	else if (r > ATMOSPHERE.top_radius)
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
	return
		scattering * RayleighPhaseFunction(nu) +
		single_mie_scattering * MiePhaseFunction(0.8, nu);
}

vec3 GetSolarRadiance()
{
  return ATMOSPHERE.solar_irradiance / (PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius);
}

void main()
{
	vec3 view_direction = normalize(view_ray);
	vec3 transmittance;
	vec3 radiance = GetSkyRadiance(camera - earth_center, view_direction, sun_direction, transmittance);

	if (dot(view_direction, sun_direction) > sun_size.y)
	{
		//radiance = radiance + transmittance * GetSolarRadiance();
	}

	o_Color.rgb = pow(vec3(1.0) - exp(-radiance / white_point * exposure), vec3(1.0 / 2.2));
	o_Color.a = 1.0;
}