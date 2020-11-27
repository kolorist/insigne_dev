#include "Sky.h"

#include <clover/Logger.h>
#include <floral/containers/fast_array.h>
#include <insigne/ut_render.h>

#include "Graphics/stb_image_write.h"

#include "InsigneImGui.h"

namespace stone
{
namespace tech
{
//-------------------------------------------------------------------

static const s32 k_transmittanceTextureWidth = 256;
static const s32 k_transmittanceTextureHeight = 64;

static const s32 k_scatteringTextureRSize = 32;
static const s32 k_scatteringTextureMuSize = 128;
static const s32 k_scatteringTextureMuSSize = 32;
static const s32 k_scatteringTextureNuSize = 8;

static const s32 k_scatteringTextureWidth = k_scatteringTextureNuSize * k_scatteringTextureMuSSize;
static const s32 k_scatteringTextureHeight = k_scatteringTextureMuSize;
static const s32 k_scatteringTextureDepth = k_scatteringTextureRSize;


//-------------------------------------------------------------------

floral::vec3f lookup_texture2d_rgb(f32* i_texture, const s32 i_x, const s32 i_y, const s32 i_width, const s32 i_height)
{

	ssize index = ((i_height - 1 - i_y) * i_width + i_x) * 3;
	f32 r = i_texture[index];
	f32 g = i_texture[index + 1];
	f32 b = i_texture[index + 2];

	return floral::vec3f(r, g, b);
}

floral::vec3f lookup_texture2d_rgb_bilinear(f32* i_texture, const floral::vec2f i_uv, const s32 i_width, const s32 i_height)
{
	f32 u = i_uv.x * i_width - 0.5f;
	f32 v = i_uv.y * i_height - 0.5f;
	s32 i = floor(u);
	s32 j = floor(v);
	u -= i;
	v -= j;
    s32 i0 = floral::max(0, floral::min(i_width - 1, i));
    s32 i1 = floral::max(0, floral::min(i_width - 1, i + 1));
    s32 j0 = floral::max(0, floral::min(i_height - 1, j));
    s32 j1 = floral::max(0, floral::min(i_height - 1, j + 1));
    return lookup_texture2d_rgb(i_texture, i0, j0, i_width, i_height) * ((1.0f - u) * (1.0f - v)) +
        lookup_texture2d_rgb(i_texture, i1, j0, i_width, i_height) * (u * (1.0f - v)) +
        lookup_texture2d_rgb(i_texture, i0, j1, i_width, i_height) * ((1.0f - u) * v) +
        lookup_texture2d_rgb(i_texture, i1, j1, i_width, i_height) * (u * v);
}

f32 get_texture_coord_from_unit_range(f32 i_x, s32 i_texSize)
{
	return 0.5f / f32(i_texSize) + i_x * (1.0f - 1.0f / f32(i_texSize));
}

f32 get_unit_range_from_texture_coord(f32 i_u, s32 i_texSize)
{
	return (i_u - 0.5f / f32(i_texSize)) / (1.0f - 1.0f / f32(i_texSize));
}

f32 get_layer_density(const DensityProfileLayer& i_layer, const f32 i_altitude)
{
	f32 density = i_layer.ExpTerm * exp(i_layer.ExpScale * i_altitude) +
		i_layer.LinearTerm * i_altitude + i_layer.ConstantTerm;
	return floral::clamp(density, 0.0f, 1.0f);
}

f32 get_profile_density(const DensityProfile& i_profile, const f32 i_altitude)
{
	return i_altitude < i_profile.Layers[0].Width ?
		get_layer_density(i_profile.Layers[0], i_altitude) :
		get_layer_density(i_profile.Layers[1], i_altitude);
}

f32 distance_to_top_atmosphere_boundary(const Atmosphere& i_atmosphere, const f32 i_r, const f32 i_mu)
{
	FLORAL_ASSERT(i_r <= i_atmosphere.TopRadius);
	FLORAL_ASSERT(i_mu >= -1.0f && i_mu <= 1.0f);

	f32 discriminant = i_r * i_r * (i_mu * i_mu - 1.0f) +
		i_atmosphere.TopRadius * i_atmosphere.TopRadius;
	f32 distance = -i_r * i_mu + floral::max(sqrtf(discriminant), 0.0f);
	return floral::max(distance, 0.0f);
}

f32 distance_to_bottom_atmosphere_boundary(const Atmosphere& i_atmosphere, const f32 i_r, const f32 i_mu)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius);
	FLORAL_ASSERT(i_mu >= -1.0f && i_mu <= 1.0f);

	f32 discriminant = i_r * i_r * (i_mu * i_mu - 1.0f) +
		i_atmosphere.BottomRadius * i_atmosphere.BottomRadius;
	f32 distance = -i_r * i_mu - floral::max(sqrtf(discriminant), 0.0f);
	return floral::max(distance, 0.0f);
}

f32 interpolate(const floral::fast_fixed_array<f32, LinearArena>& i_waveLengths,
		const floral::fast_fixed_array<f32, LinearArena>& i_waveLengthFunction,
		const f32 i_waveLength)
{
	FLORAL_ASSERT(i_waveLengths.get_size() == i_waveLengthFunction.get_size());
	if (i_waveLength < i_waveLengths[0])
	{
		return i_waveLengthFunction[0];
	}

	for (ssize i = 0; i < i_waveLengths.get_size() - 1; i++)
	{
		if (i_waveLength < i_waveLengths[i + 1])
		{
			f32 u = (i_waveLength - i_waveLengths[i]) / (i_waveLengths[i + 1] - i_waveLengths[i]);
			f32 rv = i_waveLengthFunction[i] * (1.0f - u) + i_waveLengthFunction[i + 1] * u;
			return rv;
		}
	}
	return i_waveLengthFunction[i_waveLengthFunction.get_size() - 1];
}

floral::vec3f to_rgb(const floral::fast_fixed_array<f32, LinearArena>& i_wavelengths,
		const floral::fast_fixed_array<f32, LinearArena>& i_v,
		const floral::vec3f i_lambdas, const f32 i_scale)
{
	floral::vec3f rgb;
	rgb.x = interpolate(i_wavelengths, i_v, i_lambdas.x) * i_scale;
	rgb.y = interpolate(i_wavelengths, i_v, i_lambdas.y) * i_scale;
	rgb.z = interpolate(i_wavelengths, i_v, i_lambdas.z) * i_scale;
	return rgb;
}

//-------------------------------------------------------------------

floral::vec2f get_transmittance_texture_uv_from_r_mu(const Atmosphere& i_atmosphere, const f32 i_r, const f32 i_mu)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius && i_r <= i_atmosphere.TopRadius);
	FLORAL_ASSERT(i_mu >= -1.0f && i_mu <= 1.0f);
	// Distance to top atmosphere boundary for a horizontal ray at ground level.
	f32 H = sqrtf(i_atmosphere.TopRadius * i_atmosphere.TopRadius -
			i_atmosphere.BottomRadius * i_atmosphere.BottomRadius);
	// Distance to the horizon.
	f32 rho =
		floral::max(sqrtf(i_r * i_r - i_atmosphere.BottomRadius * i_atmosphere.BottomRadius), 0.0f);
	// Distance to the top atmosphere boundary for the ray (r,mu), and its minimum
	// and maximum values over all mu - obtained for (r,1) and (r,mu_horizon).
	f32 d = distance_to_top_atmosphere_boundary(i_atmosphere, i_r, i_mu);
	f32 dMin = i_atmosphere.TopRadius - i_r;
	f32 dMax = rho + H;
	f32 xMu = (d - dMin) / (dMax - dMin);
	f32 xR = rho / H;
	return floral::vec2f(get_texture_coord_from_unit_range(xMu, k_transmittanceTextureWidth),
			get_texture_coord_from_unit_range(xR, k_transmittanceTextureHeight));
}

void get_r_mu_from_transmittance_texture_uv(const Atmosphere& i_atmosphere, const floral::vec2f& i_fragCoord, f32* o_r, f32* o_mu)
{
	FLORAL_ASSERT(i_fragCoord.x >= 0.0f && i_fragCoord.x <= 1.0f);
	FLORAL_ASSERT(i_fragCoord.y >= 0.0f && i_fragCoord.y <= 1.0f);

	f32 adjustedMu = get_unit_range_from_texture_coord(i_fragCoord.x, k_transmittanceTextureWidth);
	f32 adjustedR = get_unit_range_from_texture_coord(i_fragCoord.y, k_transmittanceTextureHeight);

	f32 H = sqrtf(i_atmosphere.TopRadius * i_atmosphere.TopRadius - i_atmosphere.BottomRadius * i_atmosphere.BottomRadius);
	f32 rho = H * adjustedR;
	f32 r = sqrtf(rho * rho + i_atmosphere.BottomRadius * i_atmosphere.BottomRadius);
	f32 dMin = i_atmosphere.TopRadius - r;
	f32 dMax = rho + H;
	f32 d = dMin + adjustedMu * (dMax - dMin);
	f32 mu = 1.0f;
	if (d != 0.0f)
	{
		mu = (H * H - rho * rho - d * d) / (2.0f * r * d);
	}
	mu = floral::clamp(mu, -1.0f, 1.0f);

	*o_r = r;
	*o_mu = mu;
}

f32 compute_optical_length_to_top_atmosphere_boundary(const Atmosphere& i_atmosphere, const DensityProfile& i_densityProfile,
		const f32 i_r, const f32 i_mu)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius && i_r <= i_atmosphere.TopRadius);
	FLORAL_ASSERT(i_mu >= -1.0f && i_mu <= 1.0f);

	const s32 k_sampleCount = 500;
	f32 dx = distance_to_top_atmosphere_boundary(i_atmosphere, i_r, i_mu) / k_sampleCount;
	f32 opticalLength = 0.0f;
	for (s32 i = 0; i < k_sampleCount; i++)
	{
		f32 di = i * dx;
		f32 ri = sqrtf(di * di + 2.0f * i_r * i_mu * di + i_r * i_r);
		f32 yi = get_profile_density(i_densityProfile, ri - i_atmosphere.BottomRadius);
		f32 w = 1.0f;
		if (i == 0 || i == k_sampleCount)
		{
			w = 0.5f;
		}
		opticalLength += yi * w * dx;
	}
	return opticalLength;
}

floral::vec3f compute_transmittance_to_top_atmosphere_boundary(const Atmosphere& i_atmosphere, const f32 i_r, const f32 i_mu)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius && i_r <= i_atmosphere.TopRadius);
	FLORAL_ASSERT(i_mu >= -1.0f && i_mu <= 1.0f);
	floral::vec3f rayleigh = i_atmosphere.RayleighScattering *
		compute_optical_length_to_top_atmosphere_boundary(i_atmosphere, i_atmosphere.RayleighDensity, i_r, i_mu);
	floral::vec3f mie = i_atmosphere.MieExtinction *
		compute_optical_length_to_top_atmosphere_boundary(i_atmosphere, i_atmosphere.MieDensity, i_r, i_mu);
	floral::vec3f absorption = i_atmosphere.AbsorptionExtinction *
		compute_optical_length_to_top_atmosphere_boundary(i_atmosphere, i_atmosphere.AbsorptionDensity, i_r, i_mu);
	f32 transmittanceR = exp(-(rayleigh.x + mie.x + absorption.x));
	f32 transmittanceG = exp(-(rayleigh.y + mie.y + absorption.y));
	f32 transmittanceB = exp(-(rayleigh.z + mie.z + absorption.z));
	return floral::vec3f(transmittanceR, transmittanceG, transmittanceB);
}

// https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/functions.glsl.html#transmittance_precomputation
floral::vec3f compute_transmittance_to_top_atmosphere_boundary_texture(const Atmosphere& i_atmosphere, const floral::vec2f& i_fragCoord)
{
	f32 r = 0.0f, mu = 0.0f;
	get_r_mu_from_transmittance_texture_uv(i_atmosphere, i_fragCoord, &r, &mu);
	return compute_transmittance_to_top_atmosphere_boundary(i_atmosphere, r, mu);
}

//-------------------------------------------------------------------

void get_r_mu_muS_nu_from_scattering_texture_uvwz(const Atmosphere& i_atmosphere, const floral::vec4f& i_uvwz,
		f32* o_r, f32* o_mu, f32* o_muS, f32* o_nu, bool* o_rayRMuIntersectsGround)
{
	assert(i_uvwz.x >= 0.0 && i_uvwz.x <= 1.0);
	assert(i_uvwz.y >= 0.0 && i_uvwz.y <= 1.0);
	assert(i_uvwz.z >= 0.0 && i_uvwz.z <= 1.0);
	assert(i_uvwz.w >= 0.0 && i_uvwz.w <= 1.0);

	f32 r = 0.0f, mu = 0.0f, muS = 0.0f, nu = 0.0f;
	bool rayRMuIntersectsGround = false;

	f32 H = sqrtf(i_atmosphere.TopRadius * i_atmosphere.TopRadius -
			i_atmosphere.BottomRadius * i_atmosphere.BottomRadius);
	f32 rho = H * get_unit_range_from_texture_coord(i_uvwz.w, k_scatteringTextureRSize);
	r = sqrt(rho * rho + i_atmosphere.BottomRadius * i_atmosphere.BottomRadius);

	if (i_uvwz.z < 0.5f)
	{
		f32 dMin = r - i_atmosphere.BottomRadius;
		f32 dMax = rho;
		f32 d = dMin + (dMax - dMin) * get_unit_range_from_texture_coord(1.0f - 2.0f * i_uvwz.z, k_scatteringTextureMuSize / 2);
		if (d == 0.0f)
		{
			mu = -1.0f;
		}
		else
		{
			mu = floral::clamp(-(rho * rho + d * d) / (2.0f * r * d), -1.0f, 1.0f);
		}
		rayRMuIntersectsGround = true;
	}
	else
	{
		f32 dMin = i_atmosphere.TopRadius - r;
		f32 dMax = rho + H;
		f32 d = dMin + (dMax - dMin) * get_unit_range_from_texture_coord(2.0f * i_uvwz.z - 1.0f, k_scatteringTextureMuSize / 2);
		if (d == 0.0f)
		{
			mu = 1.0f;
		}
		else
		{
			mu = floral::clamp((H * H - rho * rho - d * d) / (2.0f * r * d), -1.0f, 1.0f);
		}
		rayRMuIntersectsGround = false;
	}

	f32 adjustedMusS = get_unit_range_from_texture_coord(i_uvwz.y, k_scatteringTextureMuSSize);
	f32 dMin = i_atmosphere.TopRadius - i_atmosphere.BottomRadius;
	f32 dMax = H;
	f32 A = -2.0f * i_atmosphere.MuSMin * i_atmosphere.BottomRadius / (dMax - dMin);
	f32 a = (A - adjustedMusS * A) / (1.0f + adjustedMusS * A);
	f32 d = dMin + floral::min(a, A) * (dMax - dMin);
	if (d == 0)
	{
		muS = 1.0f;
	}
	else
	{
		muS = floral::clamp((H * H - d * d) / (2.0f * i_atmosphere.BottomRadius * d), -1.0f, 1.0f);
	}
	nu = floral::clamp(i_uvwz.x * 2.0f - 1.0f, -1.0f, 1.0f);

	*o_r = r;
	*o_mu = mu;
	*o_muS = muS;
	*o_nu = nu;
	*o_rayRMuIntersectsGround = rayRMuIntersectsGround;
}

void get_r_mu_muS_nu_from_scattering_fragcoord(const Atmosphere& i_atmosphere, const floral::vec3f& i_fragCoord,
		f32* o_r, f32* o_mu, f32* o_muS, f32* o_nu, bool* o_rayRMuIntersectsGround)
{
	f32 fragCoordNu = floor(i_fragCoord.x / (f32)k_scatteringTextureMuSSize);
	f32 fragCoordMuS = fmod(i_fragCoord.x, (f32)k_scatteringTextureMuSSize);

	floral::vec4f uvwz = floral::vec4f(
			fragCoordNu / f32(k_scatteringTextureNuSize - 1),
			fragCoordMuS / f32(k_scatteringTextureMuSSize),
			i_fragCoord.y / f32(k_scatteringTextureMuSize),
			i_fragCoord.z / f32(k_scatteringTextureRSize));

	f32 r = 0.0f, mu = 0.0f, muS = 0.0f, nu = 0.0f;
	bool rayRMuIntersectsGround = false;
	get_r_mu_muS_nu_from_scattering_texture_uvwz(i_atmosphere, uvwz, &r, &mu, &muS, &nu, &rayRMuIntersectsGround);
	nu = floral::clamp(nu,
			mu * muS - sqrtf((1.0f - mu * mu) * (1.0f - muS * muS)),
			mu * muS + sqrtf((1.0f - mu * mu) * (1.0f - muS * muS)));
	*o_r = r;
	*o_mu = mu;
	*o_muS = muS;
	*o_nu = nu;
	*o_rayRMuIntersectsGround = rayRMuIntersectsGround;
}

f32 distance_to_nearest_atmosphere_boundary(const Atmosphere& i_atmosphere, const f32 i_r, const f32 i_mu,
		const bool i_rayRMuIntersectsGround)
{
	if (i_rayRMuIntersectsGround)
	{
		return distance_to_bottom_atmosphere_boundary(i_atmosphere, i_r, i_mu);
	}
	else
	{
		return distance_to_top_atmosphere_boundary(i_atmosphere, i_r, i_mu);
	}
}

floral::vec3f get_transmittance_to_top_atmosphere_boundary(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture,
		const f32 i_r, const f32 i_mu)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius && i_r <= i_atmosphere.TopRadius);
	floral::vec2f uv = get_transmittance_texture_uv_from_r_mu(i_atmosphere, i_r, i_mu);
	return lookup_texture2d_rgb_bilinear(i_transmittanceTexture, uv, k_transmittanceTextureWidth, k_transmittanceTextureHeight);
}

floral::vec3f get_transmittance(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture,
		const f32 i_r, const f32 i_mu, const f32 i_d,
		const bool i_rayRMuIntersectsGround)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius && i_r <= i_atmosphere.TopRadius);
	FLORAL_ASSERT(i_mu >= -1.0f && i_mu <= 1.0f);
	FLORAL_ASSERT(i_d >= 0.0f);

	f32 rd = sqrtf(i_d * i_d + 2.0f * i_r * i_mu * i_d + i_r * i_r);
	rd = floral::clamp(rd, i_atmosphere.BottomRadius, i_atmosphere.TopRadius);
	f32 muD = (i_r * i_mu + i_d) / rd;
	muD = floral::clamp(muD, -1.0f, 1.0f);

	if (i_rayRMuIntersectsGround)
	{
		floral::vec3f t = get_transmittance_to_top_atmosphere_boundary(i_atmosphere, i_transmittanceTexture, rd, -muD) /
				get_transmittance_to_top_atmosphere_boundary(i_atmosphere, i_transmittanceTexture, i_r, -i_mu);
		floral::vec3f v(floral::min(t.x, 1.0f), floral::min(t.y, 1.0f), floral::min(t.z, 1.0f));
		return v;
	}
	else
	{
		floral::vec3f t = get_transmittance_to_top_atmosphere_boundary(i_atmosphere, i_transmittanceTexture, i_r, i_mu) /
				get_transmittance_to_top_atmosphere_boundary(i_atmosphere, i_transmittanceTexture, rd, muD);
		floral::vec3f v(floral::min(t.x, 1.0f), floral::min(t.y, 1.0f), floral::min(t.z, 1.0f));
		return v;
	}
}

floral::vec3f get_transmittance_to_sun(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture,
		const f32 i_r, const f32 i_muS)
{
	f32 sin_theta_h = i_atmosphere.BottomRadius / i_r;
	f32 cos_theta_h = -sqrtf(floral::max(1.0f - sin_theta_h * sin_theta_h, 0.0f));
	return get_transmittance_to_top_atmosphere_boundary(i_atmosphere, i_transmittanceTexture, i_r, i_muS)
		* floral::smoothstep(-sin_theta_h * i_atmosphere.SunAngularRadius,
				sin_theta_h * i_atmosphere.SunAngularRadius, i_muS - cos_theta_h);
}

void compute_single_scattering_integrand(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture,
		const f32 i_r, const f32 i_mu, const f32 i_muS,
		const f32 i_nu, const f32 i_d, const bool i_rayRMuIntersectsGround,
		floral::vec3f* o_rayleigh, floral::vec3f* o_mie)
{
	f32 rd = sqrtf(i_d * i_d + 2.0f * i_r * i_mu * i_d + i_r * i_r);
	rd = floral::clamp(rd, i_atmosphere.BottomRadius, i_atmosphere.TopRadius);
	f32 muSd = (i_r * i_muS + i_d * i_nu) / rd;
	muSd = floral::clamp(muSd, -1.0f, 1.0f);
	floral::vec3f transmittance =
		get_transmittance(i_atmosphere, i_transmittanceTexture, i_r, i_mu, i_d, i_rayRMuIntersectsGround)
		* get_transmittance_to_sun(i_atmosphere, i_transmittanceTexture, rd, muSd);
	*o_rayleigh = transmittance * get_profile_density(i_atmosphere.RayleighDensity, rd - i_atmosphere.BottomRadius);
	*o_mie = transmittance * get_profile_density(i_atmosphere.MieDensity, rd - i_atmosphere.BottomRadius);
}

void compute_single_scattering(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture,
		const f32 i_r, const f32 i_mu, const f32 i_muS, const f32 i_nu,
		const bool i_rayRMuIntersectsGround, floral::vec3f* o_rayleigh, floral::vec3f* o_mie)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius && i_r <= i_atmosphere.TopRadius);
	FLORAL_ASSERT(i_mu >= -1.0f && i_mu <= 1.0f);
	FLORAL_ASSERT(i_muS >= -1.0f && i_muS <= 1.0f);
	FLORAL_ASSERT(i_nu >= -1.0f && i_nu <= 1.0f);

	const s32 k_sampleCount = 50;
	f32 dx = distance_to_nearest_atmosphere_boundary(i_atmosphere, i_r, i_mu, i_rayRMuIntersectsGround) / k_sampleCount;

	floral::vec3f rayleighSum(0.0f, 0.0f, 0.0f);
	floral::vec3f mieSum(0.0f, 0.0f, 0.0f);
	for (s32 i = 0; i <= k_sampleCount; i++)
	{
		f32 dI = i * dx;
		floral::vec3f rayleighI, mieI;
		compute_single_scattering_integrand(i_atmosphere, i_transmittanceTexture,
				i_r, i_mu, i_muS, i_nu, dI, i_rayRMuIntersectsGround,
				&rayleighI, &mieI);
		f32 weightI = (i == 0 || i == k_sampleCount) ? 0.5f : 1.0f;
		rayleighSum += rayleighI * weightI;
		mieSum += mieI * weightI;
	}

	*o_rayleigh = rayleighSum * dx * i_atmosphere.SolarIrradiance * i_atmosphere.RayleighScattering;
	*o_mie = mieSum * dx * i_atmosphere.SolarIrradiance * i_atmosphere.MieScattering;
}

// https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/functions.glsl.html#single_scattering_precomputation
void compute_single_scattering_texture(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture, const floral::vec3f& i_fragCoord, floral::vec3f* o_rayleigh, floral::vec3f* o_mie)
{
	f32 r = 0.0f, mu = 0.0f, muS = 0.0f, nu = 0.0f;
	bool rayRMuIntersectsGround = false;
	get_r_mu_muS_nu_from_scattering_fragcoord(i_atmosphere, i_fragCoord, &r, &mu, &muS, &nu, &rayRMuIntersectsGround);
	compute_single_scattering(i_atmosphere, i_transmittanceTexture, r, mu, muS, nu, rayRMuIntersectsGround, o_rayleigh, o_mie);
}

//-------------------------------------------------------------------

Sky::Sky()
{
}

Sky::~Sky()
{
}

ICameraMotion* Sky::GetCameraMotion()
{
	return nullptr;
}

const_cstr Sky::GetName() const
{
	return k_name;
}

void Sky::_OnInitialize()
{
	m_DataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));
	m_TemporalArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));

	const f32 k_lengthUnitInMeters = 1000.0f;

	// sky model: spectrum
	const s32 k_lambdaMin = 360; // nanometers
	const s32 k_lambdaMax = 830; // nanometers
	const s32 k_lambdaCount = (k_lambdaMax - k_lambdaMin) / 10 + 1;
	const f32 k_lambdaR = 680.0f;
	const f32 k_lambdaG = 550.0f;
	const f32 k_lambdaB = 440.0f;

	floral::fast_fixed_array<f32, LinearArena> wavelengths(m_DataArena);
	wavelengths.reserve(k_lambdaCount);
	for (s32 l = k_lambdaMin; l <= k_lambdaMax; l += 10)
	{
		wavelengths.push_back(l);
	}

	// sky model: rayleigh
	const f32 k_rayleigh = 1.24062e-6f;
	const f32 k_rayleighScaleHeight = 8000.0f;

	floral::fast_fixed_array<f32, LinearArena> rayleighScattering(m_DataArena);
	// compute Rayleigh scattering for each spectrum
	rayleighScattering.reserve(k_lambdaCount);
	for (s32 l = k_lambdaMin; l <= k_lambdaMax; l += 10)
	{
		f32 lambda = (f32)l * 1e-3; // convert to micrometers
		f32 r = k_rayleigh * powf((f32)lambda, -4.0f);
		rayleighScattering.push_back(r);
	}

	// sky model: mie
	const f32 k_mieScaleHeight = 1200.0f;
	const f32 k_mieAngstromAlpha = 0.0f;
	const f32 k_mieAngstromBeta = 5.328e-3f;
	const f32 k_mieSingleScatteringAlbedo = 0.9f;
	const f32 k_miePhaseFunctionG = 0.8f;

	floral::fast_fixed_array<f32, LinearArena> mieScattering(m_DataArena);
	floral::fast_fixed_array<f32, LinearArena> mieExtinction(m_DataArena);
	mieScattering.reserve(k_lambdaCount);
	mieExtinction.reserve(k_lambdaCount);
	for (int l = k_lambdaMin; l <= k_lambdaMax; l += 10)
	{
		f32 lambda = (f32)l * 1e-3; // convert to micrometers
		f32 mie = k_mieAngstromBeta / k_mieScaleHeight * powf(lambda, -k_mieAngstromAlpha);
		f32 mieScatter = mie * k_mieSingleScatteringAlbedo;
		mieScattering.push_back(mieScatter);
		mieExtinction.push_back(mie);
	}

	// sky model: absorption
	const f32 k_ozoneCrossSection[48] = {
		1.18e-27f, 2.182e-28f, 2.818e-28f, 6.636e-28f, 1.527e-27f, 2.763e-27f, 5.52e-27f,
		8.451e-27f, 1.582e-26f, 2.316e-26f, 3.669e-26f, 4.924e-26f, 7.752e-26f, 9.016e-26f,
		1.48e-25f, 1.602e-25f, 2.139e-25f, 2.755e-25f, 3.091e-25f, 3.5e-25f, 4.266e-25f,
		4.672e-25f, 4.398e-25f, 4.701e-25f, 5.019e-25f, 4.305e-25f, 3.74e-25f, 3.215e-25f,
		2.662e-25f, 2.238e-25f, 1.852e-25f, 1.473e-25f, 1.209e-25f, 9.423e-26f, 7.455e-26f,
		6.566e-26f, 5.105e-26f, 4.15e-26f, 4.228e-26f, 3.237e-26f, 2.451e-26f, 2.801e-26f,
		2.534e-26f, 1.624e-26f, 1.465e-26f, 2.078e-26f, 1.383e-26f, 7.105e-27f
	};
	const f32 k_dobsonUnit = 2.687e20f;
	const f32 k_maxOzoneNumberDensity = 300.0f * k_dobsonUnit / 15000.0f;
	const bool k_useOzone = true;
	floral::fast_fixed_array<f32, LinearArena> absorptionExtinction(m_DataArena);
	absorptionExtinction.reserve(k_lambdaCount);
	for (int l = k_lambdaMin; l <= k_lambdaMax; l += 10)
	{
		if (k_useOzone)
		{
			f32 ozoneExtinction = k_maxOzoneNumberDensity * k_ozoneCrossSection[(l - k_lambdaMin) / 10];
			absorptionExtinction.push_back(ozoneExtinction);
		}
		else
		{
			absorptionExtinction.push_back(0.0f);
		}
	}

	// model: initialization
	m_Atmosphere.TopRadius = 6420.0f;
	m_Atmosphere.BottomRadius = 6360.0f;
	m_Atmosphere.MuSMin = -0.207912f;
	m_Atmosphere.SolarIrradiance = floral::vec3f(1.474000f, 1.850400f, 1.911980f); // TODO (compute)
	m_Atmosphere.SunAngularRadius = 0.004675f;
	m_Atmosphere.RayleighScattering = to_rgb(wavelengths, rayleighScattering,
			floral::vec3f(k_lambdaR, k_lambdaG, k_lambdaB), k_lengthUnitInMeters);
	m_Atmosphere.RayleighDensity.Layers[0] = DensityProfileLayer {
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f
	};
	m_Atmosphere.RayleighDensity.Layers[1] = DensityProfileLayer {
		0.0f / k_lengthUnitInMeters, 1.0f, -1.0f / k_rayleighScaleHeight * k_lengthUnitInMeters, 0.0f * k_lengthUnitInMeters, 0.0f
	};

	m_Atmosphere.MieScattering = floral::vec3f(0.003996f, 0.003996f, 0.003996f); // TODO (compute)
	m_Atmosphere.MieExtinction = to_rgb(wavelengths, mieExtinction,
			floral::vec3f(k_lambdaR, k_lambdaG, k_lambdaB), k_lengthUnitInMeters);
	m_Atmosphere.MieDensity.Layers[0] = DensityProfileLayer {
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f
	};
	m_Atmosphere.MieDensity.Layers[1] = DensityProfileLayer {
		0.0f / k_lengthUnitInMeters, 1.0f, -1.0f / k_mieScaleHeight * k_lengthUnitInMeters, 0.0f * k_lengthUnitInMeters, 0.0f
	};

	m_Atmosphere.AbsorptionExtinction = to_rgb(wavelengths, absorptionExtinction,
			floral::vec3f(k_lambdaR, k_lambdaG, k_lambdaB), k_lengthUnitInMeters);
	m_Atmosphere.AbsorptionDensity.Layers[0] = DensityProfileLayer {
		25000.0f/ k_lengthUnitInMeters, 0.0f, 0.0f * k_lengthUnitInMeters, 1.0f / 15000.0f * k_lengthUnitInMeters, -2.0f / 3.0f
	};
	m_Atmosphere.AbsorptionDensity.Layers[1] = DensityProfileLayer {
		0.0f / k_lengthUnitInMeters, 0.0f, 0.0f * k_lengthUnitInMeters, -1.0f / 15000.0f * k_lengthUnitInMeters, 8.0f / 3.0f
	};

	// TODO: note texture directions
	m_TemporalArena->free_all();
	ssize transmittanceTextureSizeBytes = k_transmittanceTextureWidth * k_transmittanceTextureHeight * 3 * sizeof(f32);
	f32* transmittanceTexture = (f32*)m_TemporalArena->allocate(transmittanceTextureSizeBytes);
	memset(transmittanceTexture, 0, transmittanceTextureSizeBytes);

	{
		for (s32 v = 0; v < k_transmittanceTextureHeight; v++)
		{
			for (s32 u = 0; u < k_transmittanceTextureWidth; u++)
			{
				floral::vec2f fragCoord(
						((f32)u + 0.5f) / (f32)k_transmittanceTextureWidth,
						((f32)v + 0.5f) / (f32)k_transmittanceTextureHeight);
				floral::vec3f transmittance = compute_transmittance_to_top_atmosphere_boundary_texture(m_Atmosphere, fragCoord);
				FLORAL_ASSERT(transmittance.x > 0.0f && transmittance.x <= 1.0f);
				FLORAL_ASSERT(transmittance.y > 0.0f && transmittance.y <= 1.0f);
				FLORAL_ASSERT(transmittance.z > 0.0f && transmittance.z <= 1.0f);
				s32 pidx = ((k_transmittanceTextureHeight - 1 - v) * k_transmittanceTextureWidth + u) * 3;
				transmittanceTexture[pidx] = transmittance.x;
				transmittanceTexture[pidx + 1] = transmittance.y;
				transmittanceTexture[pidx + 2] = transmittance.z;
			}
		}
		// TODO: check this
		stbi_write_hdr("transmittance.hdr", k_transmittanceTextureWidth, k_transmittanceTextureHeight,
				3, transmittanceTexture);
	}

	{
		ssize texSizeBytes = k_scatteringTextureWidth * k_scatteringTextureHeight * 3 * sizeof(f32);
		f32* rayleighTexture = (f32*)m_TemporalArena->allocate(texSizeBytes);
		f32* mieTexture = (f32*)m_TemporalArena->allocate(texSizeBytes);
		for (s32 w = 0; w < k_scatteringTextureDepth; w++)
		{
			memset(rayleighTexture, 0, texSizeBytes);
			memset(mieTexture, 0, texSizeBytes);
			for (s32 v = 0; v < k_scatteringTextureHeight; v++)
			{
				for (s32 u = 0; u < k_scatteringTextureWidth; u++)
				{
					floral::vec3f fragCoord(((f32)u + 0.5f), ((f32)v + 0.5f), ((f32)w + 0.5f));
					floral::vec3f rayleigh;
					floral::vec3f mie;
					compute_single_scattering_texture(m_Atmosphere, transmittanceTexture, fragCoord, &rayleigh, &mie);

					s32 pidx = (v * k_scatteringTextureWidth + u) * 3;
					rayleighTexture[pidx] = rayleigh.x;
					rayleighTexture[pidx + 1] = rayleigh.y;
					rayleighTexture[pidx + 2] = rayleigh.z;

					mieTexture[pidx] = mie.x;
					mieTexture[pidx + 1] = mie.y;
					mieTexture[pidx + 2] = mie.z;
				}
			}
			c8 name[128];
			sprintf(name, "ss_rayleigh_%d.hdr", w);
			stbi_write_hdr(name, k_scatteringTextureWidth, k_scatteringTextureHeight, 3, rayleighTexture);
			sprintf(name, "ss_mie_%d.hdr", w);
			stbi_write_hdr(name, k_scatteringTextureWidth, k_scatteringTextureHeight, 3, mieTexture);
			CLOVER_DEBUG("#%d: done", w);
		}
	}
}

void Sky::_OnUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("Controller");
	ImGui::End();
}

void Sky::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void Sky::_OnCleanUp()
{
	g_StreammingAllocator.free(m_TemporalArena);
	g_StreammingAllocator.free(m_DataArena);
}

//-------------------------------------------------------------------
}
}
