#include "Sky.h"

#include <clover/Logger.h>

#include <floral/io/filesystem.h>
#include <floral/containers/fast_array.h>

#include <insigne/ut_render.h>

#include "Graphics/stb_image_write.h"
#include "precomputed_sky.h"

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

static const s32 k_irrandianceTextureWidth = 64;
static const s32 k_irrandianceTextureHeight = 16;

//-------------------------------------------------------------------

floral::vec3f lookup_texture2d_rgb(f32* i_texture, const s32 i_x, const s32 i_y, const s32 i_width, const s32 i_height)
{
	ssize index = (i_y * i_width + i_x) * 3;
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

floral::vec3f lookup_texture3d_rgb(f32** i_texture, const s32 i_x, const s32 i_y, const s32 i_z,
		const s32 i_width, const s32 i_height, const s32 i_depth)
{
	f32* texture = i_texture[i_z];
	ssize index = (i_y * i_width + i_x) * 3;
	f32 r = texture[index];
	f32 g = texture[index + 1];
	f32 b = texture[index + 2];

	return floral::vec3f(r, g, b);
}

floral::vec3f lookup_texture3d_rgb_bilinear(f32** i_texture, const floral::vec3f i_uvw, const s32 i_width, const s32 i_height, const s32 i_depth)
{
	f32 u = i_uvw.x * i_width - 0.5f;
	f32 v = i_uvw.y * i_height - 0.5f;
	f32 w = i_uvw.z * i_depth - 0.5f;
	s32 i = floor(u);
	s32 j = floor(v);
	s32 k = floor(w);
	u -= i;
	v -= j;
	w -= k;
	s32 i0 = floral::max(0, floral::min(i_width - 1, i));
	s32 i1 = floral::max(0, floral::min(i_width - 1, i + 1));
	s32 j0 = floral::max(0, floral::min(i_height - 1, j));
	s32 j1 = floral::max(0, floral::min(i_height - 1, j + 1));
	s32 k0 = floral::max(0, floral::min(i_depth - 1, k));
	s32 k1 = floral::max(0, floral::min(i_depth - 1, k + 1));
	return lookup_texture3d_rgb(i_texture, i0, j0, k0, i_width, i_height, i_depth) * ((1.0f - u) * (1.0f - v) * (1.0f - w)) +
		lookup_texture3d_rgb(i_texture, i1, j0, k0, i_width, i_height, i_depth) * (u * (1.0f - v) * (1.0f - w)) +
		lookup_texture3d_rgb(i_texture, i0, j1, k0, i_width, i_height, i_depth) * ((1.0f - u) * v * (1.0f - w)) +
		lookup_texture3d_rgb(i_texture, i1, j1, k0, i_width, i_height, i_depth) * (u * v * (1.0f - w)) +
		lookup_texture3d_rgb(i_texture, i0, j0, k1, i_width, i_height, i_depth) * ((1.0f - u) * (1.0f - v) * w) +
		lookup_texture3d_rgb(i_texture, i1, j0, k1, i_width, i_height, i_depth) * (u * (1.0f - v) * w) +
		lookup_texture3d_rgb(i_texture, i0, j1, k1, i_width, i_height, i_depth) * ((1.0f - u) * v * w) +
		lookup_texture3d_rgb(i_texture, i1, j1, k1, i_width, i_height, i_depth) * (u * v * w);
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

const bool ray_intersects_ground(const Atmosphere& i_atmosphere, const f32 i_r, const f32 i_mu)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius);
	FLORAL_ASSERT(i_mu >= -1.0f && i_mu <= 1.0f);
	return i_mu < 0.0f && i_r * i_r * (i_mu * i_mu - 1.0f) +
		i_atmosphere.BottomRadius * i_atmosphere.BottomRadius >= 0.0f;
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

const floral::vec2f get_irradiance_texture_uv_from_r_muS(const Atmosphere& i_atmosphere, const f32 i_r, const f32 i_muS)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius && i_r <= i_atmosphere.TopRadius);
	FLORAL_ASSERT(i_muS >= -1.0f && i_muS <= 1.0f);
	f32 xR = (i_r - i_atmosphere.BottomRadius) / (i_atmosphere.TopRadius - i_atmosphere.BottomRadius);
	f32 xMuS = i_muS * 0.5f + 0.5f;
	return floral::vec2f(get_texture_coord_from_unit_range(xMuS, k_irrandianceTextureWidth),
			get_texture_coord_from_unit_range(xR, k_irrandianceTextureHeight));
}

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

void get_r_mu_from_transmittance_texture_uv(const Atmosphere& i_atmosphere, const floral::vec2f& i_uv, f32* o_r, f32* o_mu)
{
	FLORAL_ASSERT(i_uv.x >= 0.0f && i_uv.x <= 1.0f);
	FLORAL_ASSERT(i_uv.y >= 0.0f && i_uv.y <= 1.0f);

	f32 adjustedMu = get_unit_range_from_texture_coord(i_uv.x, k_transmittanceTextureWidth);
	f32 adjustedR = get_unit_range_from_texture_coord(i_uv.y, k_transmittanceTextureHeight);

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
	get_r_mu_from_transmittance_texture_uv(i_atmosphere,
			i_fragCoord / floral::vec2f(k_transmittanceTextureWidth, k_transmittanceTextureHeight), &r, &mu);
	return compute_transmittance_to_top_atmosphere_boundary(i_atmosphere, r, mu);
}

//-------------------------------------------------------------------

const floral::vec4f get_scattering_texture_uvwz_from_r_mu_muS_nu(const Atmosphere& i_atmosphere,
		const f32 i_r, const f32 i_mu, const f32 i_muS, const f32 i_nu,
		const bool i_rayRMuIntersectsGround)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius && i_r <= i_atmosphere.TopRadius);
	FLORAL_ASSERT(i_mu >= -1.0f && i_mu <= 1.0f);
	FLORAL_ASSERT(i_muS >= -1.0f && i_muS <= 1.0f);
	FLORAL_ASSERT(i_nu >= -1.0f && i_nu <= 1.0f);

	// Distance to top i_atmospheratmosphere boundary for a horizontal ray at ground level.
	f32 H = sqrtf(i_atmosphere.TopRadius * i_atmosphere.TopRadius - i_atmosphere.BottomRadius * i_atmosphere.BottomRadius);
	// Distance to the horizon.
	f32 rho =
		floral::max(sqrtf(i_r * i_r - i_atmosphere.BottomRadius * i_atmosphere.BottomRadius), 0.0f);
	f32 ur = get_texture_coord_from_unit_range(rho / H, k_scatteringTextureRSize);

	// Discriminant of the quadratic equation for the intersections of the ray
	// (i_r,i_mu) with the ground (see RayIntersectsGround).
	f32 rMu = i_r * i_mu;
	f32 discriminant = rMu * rMu - i_r * i_r + i_atmosphere.BottomRadius * i_atmosphere.BottomRadius;
	f32 uMu = 0.0f;
	if (i_rayRMuIntersectsGround)
	{
		// Distance to the ground for the ray (i_r,i_mu), and its minimum and maximum
		// values over all i_mu - obtained for (i_r,-1) and (i_r,mu_horizon).
		f32 d = -rMu - floral::max(sqrtf(discriminant), 0.0f);
		f32 dMin = i_r - i_atmosphere.BottomRadius;
		f32 dMax = rho;
		uMu = 0.5f - 0.5f * get_texture_coord_from_unit_range(dMax == dMin ? 0.0f :
				(d - dMin) / (dMax - dMin), k_scatteringTextureMuSize / 2);
	}
	else
	{
		// Distance to the top i_atmosphere boundary for the ray (i_r,i_mu), and its
		// minimum and maximum values over all i_mu - obtained for (i_r,1) and
		// (i_r,mu_horizon).
		f32 d = -rMu + floral::max(sqrtf(discriminant + H * H), 0.0f);
		f32 dMin = i_atmosphere.TopRadius - i_r;
		f32 dMax = rho + H;
		uMu = 0.5f + 0.5f * get_texture_coord_from_unit_range((d - dMin) / (dMax - dMin), k_scatteringTextureMuSize / 2);
	}

	f32 d = distance_to_top_atmosphere_boundary(i_atmosphere, i_atmosphere.BottomRadius, i_muS);
	f32 dMin = i_atmosphere.TopRadius - i_atmosphere.BottomRadius;
	f32 dMax = H;
	f32 a = (d - dMin) / (dMax - dMin);
	f32 D = distance_to_top_atmosphere_boundary(i_atmosphere, i_atmosphere.BottomRadius, i_atmosphere.MuSMin);
	f32 A = (D - dMin) / (dMax - dMin);
	// An ad-hoc function equal to 0 for i_muS = mu_s_min (because then d = D and
	// thus a = A), equal to 1 for i_muS = 1 (because then d = dMin and thus
	// a = 0), and with a large slope around i_muS = 0, to get more texture
	// samples near the horizon.
	f32 uMuS = get_texture_coord_from_unit_range(floral::max(1.0f - a / A, 0.0f) / (1.0f + a), k_scatteringTextureMuSSize);
	f32 uNu = (i_nu + 1.0f) / 2.0f;
	return floral::vec4f(uNu, uMuS, uMu, ur);
}

void get_r_mu_muS_nu_from_scattering_texture_uvwz(const Atmosphere& i_atmosphere, const floral::vec4f& i_uvwz,
		f32* o_r, f32* o_mu, f32* o_muS, f32* o_nu, bool* o_rayRMuIntersectsGround)
{
	FLORAL_ASSERT(i_uvwz.x >= 0.0 && i_uvwz.x <= 1.0);
	FLORAL_ASSERT(i_uvwz.y >= 0.0 && i_uvwz.y <= 1.0);
	FLORAL_ASSERT(i_uvwz.z >= 0.0 && i_uvwz.z <= 1.0);
	FLORAL_ASSERT(i_uvwz.w >= 0.0 && i_uvwz.w <= 1.0);

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

void get_r_mu_muS_nu_from_scattering_texture_fragcoord(const Atmosphere& i_atmosphere, const floral::vec3f& i_fragCoord,
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
	get_r_mu_muS_nu_from_scattering_texture_fragcoord(i_atmosphere, i_fragCoord, &r, &mu, &muS, &nu, &rayRMuIntersectsGround);
	compute_single_scattering(i_atmosphere, i_transmittanceTexture, r, mu, muS, nu, rayRMuIntersectsGround, o_rayleigh, o_mie);
}

const f32 rayleigh_phase_function(f32 i_nu)
{
	f32 k = 3.0f / (16.0f * floral::pi);
	return k * (1.0f + i_nu * i_nu);
}

const f32 mie_phase_function(f32 i_g, f32 i_nu)
{
	f32 k = 3.0f / (8.0f * floral::pi) * (1.0f - i_g * i_g) / (2.0f + i_g * i_g);
	return k * (1.0f + i_nu * i_nu) / powf(1.0f + i_g * i_g - 2.0f * i_g * i_nu, 1.5f);
}

const floral::vec3f get_scattering(const Atmosphere& i_atmosphere, f32** i_scatteringTexture,
		const f32 i_r, const f32 i_mu, const f32 i_muS, const f32 i_nu,
		const bool i_rayRMuIntersectsGround)
{
	floral::vec4f uvwz = get_scattering_texture_uvwz_from_r_mu_muS_nu(
			i_atmosphere, i_r, i_mu, i_muS, i_nu, i_rayRMuIntersectsGround);
	f32 texCoordX = uvwz.x * f32(k_scatteringTextureNuSize- 1);
	f32 texX = floor(texCoordX);
	f32 lerp = texCoordX - texX;
	floral::vec3f uvw0((texX + uvwz.y) / f32(k_scatteringTextureNuSize), uvwz.z, uvwz.w);
	floral::vec3f uvw1((texX + 1.0f + uvwz.y) / f32(k_scatteringTextureNuSize), uvwz.z, uvwz.w);
	return lookup_texture3d_rgb_bilinear(i_scatteringTexture, uvw0, k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth) * (1.0f - lerp) +
		lookup_texture3d_rgb_bilinear(i_scatteringTexture, uvw1, k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth) * lerp;
}

const floral::vec3f get_scattering(const Atmosphere& i_atmosphere,
		f32** i_singleRayleighScatteringTexture, f32** i_singleMieScatteringTexture, f32** i_multipleScaterringTexture,
		const f32 i_r, const f32 i_mu, const f32 i_muS, const f32 i_nu,
		const bool i_rayRMuIntersectsGround, const s32 i_scatteringOrder)
{
	if (i_scatteringOrder == 1)
	{
		floral::vec3f rayleigh = get_scattering(i_atmosphere, i_singleRayleighScatteringTexture,
				i_r, i_mu, i_muS, i_nu, i_rayRMuIntersectsGround);
		floral::vec3f mie = get_scattering(i_atmosphere, i_singleMieScatteringTexture,
				i_r, i_mu, i_muS, i_nu, i_rayRMuIntersectsGround);
		return rayleigh * rayleigh_phase_function(i_nu) + mie * mie_phase_function(i_atmosphere.MiePhaseFunctionG, i_nu);
	}
	else
	{
		return get_scattering(i_atmosphere, i_multipleScaterringTexture, i_r, i_mu, i_muS, i_nu, i_rayRMuIntersectsGround);
	}
}

const floral::vec3f get_irradiance(const Atmosphere& i_atmosphere, f32* i_irradianceTexture, const f32 i_r, const f32 i_muS)
{
	floral::vec2f uv = get_irradiance_texture_uv_from_r_muS(i_atmosphere, i_r, i_muS);
	return lookup_texture2d_rgb_bilinear(i_irradianceTexture, uv, k_irrandianceTextureWidth, k_irrandianceTextureHeight);
}

const floral::vec3f compute_scattering_density(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture,
		f32** i_singleRayleighScatteringTexture, f32** i_singleMieScatteringTexture, f32** i_multipleScaterringTexture,
		f32* i_irradianceTexture, const f32 i_r, const f32 i_mu, const f32 i_muS, const f32 i_nu, const s32 i_scatteringOrder)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius && i_r <= i_atmosphere.TopRadius);
	FLORAL_ASSERT(i_mu >= -1.0f && i_mu <= 1.0f);
	FLORAL_ASSERT(i_muS >= -1.0f && i_muS <= 1.0f);
	FLORAL_ASSERT(i_nu >= -1.0f && i_nu <= 1.0f);
	FLORAL_ASSERT(i_scatteringOrder >= 2);

	// Compute unit direction vectors for the zenith, the view direction omega and
	// and the sun direction omega_s, such that the cosine of the view-zenith
	// angle is mu, the cosine of the sun-zenith angle is i_muS, and the cosine of
	// the view-sun angle is nu. The goal is to simplify computations below.
	floral::vec3f zenithDirection(0.0f, 0.0f, 1.0f);
	floral::vec3f omega(sqrtf(1.0f - i_mu * i_mu), 0.0f, i_mu);
	f32 sunDirX = omega.x == 0.0f ? 0.0f : (i_nu - i_mu * i_muS) / omega.x;
	f32 sunDirY = sqrtf(floral::max(1.0f - sunDirX * sunDirX - i_muS * i_muS, 0.0f));
	floral::vec3f omegaS(sunDirX, sunDirY, i_muS);

	const s32 k_SampleCount = 16;
	const f32 dphi = floral::pi / f32(k_SampleCount); // radians
	const f32 dtheta = floral::pi / f32(k_SampleCount); // radians
	floral::vec3f rayleighMie(0.0f, 0.0f, 0.0f);

	// Nested loops for the integral over all the incident directions omega_i.
	for (s32 l = 0; l < k_SampleCount; ++l)
	{
		f32 theta = (f32(l) + 0.5) * dtheta;
		f32 cosTheta = cos(theta);
		f32 sinTheta = sin(theta);
		bool rayRThetaIntersectsGround = ray_intersects_ground(i_atmosphere, i_r, cosTheta);

		// The distance and transmittance to the ground only depend on theta, so we
		// can compute them in the outer loop for efficiency.
		f32 distanceToGround = 0.0f;
		floral::vec3f transmittanceToGround(0.0f, 0.0f, 0.0f);
		floral::vec3f groundAlbedo(0.0f, 0.0f, 0.0f);
		if (rayRThetaIntersectsGround)
		{
			distanceToGround = distance_to_bottom_atmosphere_boundary(i_atmosphere, i_r, cosTheta);
			transmittanceToGround =
				get_transmittance(i_atmosphere, i_transmittanceTexture, i_r, cosTheta,
						distanceToGround, true /* ray_intersects_ground */);
			groundAlbedo = i_atmosphere.GroundAlbedo;
		}

		for (s32 m = 0; m < 2 * k_SampleCount; ++m)
		{
			f32 phi = (f32(m) + 0.5f) * dphi;
			floral::vec3f omegaI(cosf(phi) * sinTheta, sinf(phi) * sinTheta, cosTheta);
			f32 domegaI = dtheta * dphi * sinf(theta);

			// The radiance L_i arriving from direction omega_i after n-1 bounces is
			// the sum of a term given by the precomputed scattering texture for the
			// (n-1)-th order:
			f32 nu1 = floral::dot(omegaS, omegaI);
			floral::vec3f incidentRadiance = get_scattering(i_atmosphere,
					i_singleRayleighScatteringTexture, i_singleMieScatteringTexture,
					i_multipleScaterringTexture, i_r, omegaI.z, i_muS, nu1,
					rayRThetaIntersectsGround, i_scatteringOrder - 1);

			// and of the contribution from the light paths with n-1 bounces and whose
			// last bounce is on the ground. This contribution is the product of the
			// transmittance to the ground, the ground albedo, the ground BRDF, and
			// the irradiance received on the ground after n-2 bounces.
			floral::vec3f tmp = zenithDirection * i_r + omegaI * distanceToGround;
			floral::vec3<f64> tmpHighPrecision(tmp.x, tmp.y, tmp.z);
			floral::vec3<f64> tmpHighPrecisionNormalized = floral::normalize(tmpHighPrecision);

			floral::vec3f groundNormal((f32)tmpHighPrecisionNormalized.x, (f32)tmpHighPrecisionNormalized.y, (f32)tmpHighPrecisionNormalized.z);
			floral::vec3f groundIrradiance = get_irradiance(
					i_atmosphere, i_irradianceTexture, i_atmosphere.BottomRadius,
					floral::dot(groundNormal, omegaS));
			incidentRadiance += transmittanceToGround * groundAlbedo * (1.0f / floral::pi) * groundIrradiance;

			// The radiance finally scattered from direction omega_i towards direction
			// -omega is the product of the incident radiance, the scattering
			// coefficient, and the phase function for directions omega and omega_i
			// (all this summed over all particle types, i.e. Rayleigh and Mie).
			f32 nu2 = floral::dot(omega, omegaI);
			f32 rayleighDensity = get_profile_density(i_atmosphere.RayleighDensity, i_r - i_atmosphere.BottomRadius);
			f32 mieDensity = get_profile_density(i_atmosphere.MieDensity, i_r - i_atmosphere.BottomRadius);
			rayleighMie += incidentRadiance * (i_atmosphere.RayleighScattering * rayleighDensity *
					rayleigh_phase_function(nu2) + i_atmosphere.MieScattering * mieDensity *
					mie_phase_function(i_atmosphere.MiePhaseFunctionG, nu2)) * domegaI;
		}
	}
	return rayleighMie;
}


// https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/functions.glsl.html#multiple_scattering_precomputation
const floral::vec3f compute_scattering_density_texture(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture,
		f32** i_singleRayleighScatteringTexture, f32** i_singleMieScatteringTexture, f32** i_multipleScaterringTexture,
		f32* i_irradianceTexture, const floral::vec3f& i_fragCoord, const s32 i_scatteringOrder)
{
	f32 r = 0.0f, mu = 0.0f, muS = 0.0f, nu = 0.0f;
	bool rayRMuIntersectsGround = false;
	get_r_mu_muS_nu_from_scattering_texture_fragcoord(i_atmosphere, i_fragCoord, &r, &mu, &muS, &nu, &rayRMuIntersectsGround);
	return compute_scattering_density(i_atmosphere, i_transmittanceTexture,
			i_singleRayleighScatteringTexture, i_singleMieScatteringTexture,
			i_multipleScaterringTexture, i_irradianceTexture, r, mu, muS, nu, i_scatteringOrder);
}

void get_r_muS_from_irradiance_texture_uv(const Atmosphere& i_atmosphere, const floral::vec2f& i_uv, f32* o_r, f32* o_muS)
{
	FLORAL_ASSERT(i_uv.x >= 0.0f && i_uv.x <= 1.0f);
	FLORAL_ASSERT(i_uv.y >= 0.0f && i_uv.y <= 1.0f);
	f32 xMuS = get_unit_range_from_texture_coord(i_uv.x, k_irrandianceTextureWidth);
	f32 xR = get_unit_range_from_texture_coord(i_uv.y, k_irrandianceTextureHeight);
	*o_r = i_atmosphere.BottomRadius + xR * (i_atmosphere.TopRadius - i_atmosphere.BottomRadius);
	*o_muS = floral::clamp(2.0f * xMuS - 1.0f, -1.0f, 1.0f);
}

floral::vec3f compute_direct_irradiance(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture, const f32 i_r, const f32 i_muS)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius && i_r <= i_atmosphere.TopRadius);
	FLORAL_ASSERT(i_muS >= -1.0f && i_muS <= 1.0f);

	f32 alphaS = i_atmosphere.SunAngularRadius;
	// Approximate average of the cosine factor i_muS over the visible fraction of
	// the Sun disc.
	f32 averageCosineFactor =
		i_muS < -alphaS ? 0.0f : (i_muS > alphaS ? i_muS : (i_muS + alphaS) * (i_muS + alphaS) / (4.0f * alphaS));

	return i_atmosphere.SolarIrradiance *
		get_transmittance_to_top_atmosphere_boundary(i_atmosphere, i_transmittanceTexture, i_r, i_muS) * averageCosineFactor;
}

// https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/functions.glsl.html#irradiance
floral::vec3f compute_direct_irradiance_texture(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture, const floral::vec2f& i_fragCoord)
{
	f32 r = 0.0f, muS = 0.0f;
	get_r_muS_from_irradiance_texture_uv(i_atmosphere,
			i_fragCoord / floral::vec2f(k_irrandianceTextureWidth, k_irrandianceTextureHeight), &r, &muS);
	return compute_direct_irradiance(i_atmosphere, i_transmittanceTexture, r, muS);
}

floral::vec3f compute_indirect_irradiance(const Atmosphere& i_atmosphere,
		f32** i_singleRayleighScatteringTexture, f32** i_singleMieScatteringTexture, f32** i_multipleScatteringTexture,
		const f32 i_r, const f32 i_muS, const s32 i_scatteringOrder)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius && i_r <= i_atmosphere.TopRadius);
	FLORAL_ASSERT(i_muS >= -1.0f && i_muS <= 1.0f);
	FLORAL_ASSERT(i_scatteringOrder >= 1);

	const s32 k_sampleCount = 32;
	const f32 dphi = floral::pi / f32(k_sampleCount);
	const f32 dtheta = floral::pi / f32(k_sampleCount);

	floral::vec3f result(0.0f, 0.0f, 0.0f);
	floral::vec3f omegaS(sqrtf(1.0f - i_muS * i_muS), 0.0f, i_muS);
	for (s32 j = 0; j < k_sampleCount / 2; ++j)
	{
		f32 theta = (f32(j) + 0.5f) * dtheta;
		for (s32 i = 0; i < 2 * k_sampleCount; ++i)
		{
			f32 phi = (f32(i) + 0.5f) * dphi;
			floral::vec3f omega(cosf(phi) * sinf(theta), sinf(phi) * sinf(theta), cosf(theta));
			f32 domega = dtheta * dphi * sinf(theta);

			f32 nu = dot(omega, omegaS);
			result += get_scattering(i_atmosphere,
					i_singleRayleighScatteringTexture, i_singleMieScatteringTexture, i_multipleScatteringTexture,
					i_r, omega.z, i_muS, nu, false /* i_rayRMuIntersectsGround */,
					i_scatteringOrder) * omega.z * domega;
		}
	}
	return result;
}

floral::vec3f compute_indirect_irradiance_texture(const Atmosphere& i_atmosphere,
		f32** i_singleRayleighScatteringTexture, f32** i_singleMieScatteringTexture, f32** i_multipleScatteringTexture,
		const floral::vec2f& i_fragCoord, const s32 i_scatteringOrder)
{
	f32 r = 0.0f, muS = 0.0f;
	get_r_muS_from_irradiance_texture_uv(i_atmosphere,
			i_fragCoord / floral::vec2f(k_irrandianceTextureWidth, k_irrandianceTextureHeight), &r, &muS);
	return compute_indirect_irradiance(i_atmosphere,
			i_singleRayleighScatteringTexture, i_singleMieScatteringTexture, i_multipleScatteringTexture,
			r, muS, i_scatteringOrder);
}

const floral::vec3f compute_multiple_scattering(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture,
		f32** i_scatteringDensityTexture, const f32 i_r, const f32 i_mu, const f32 i_muS, const f32 i_nu, const bool i_rayRMuIntersectsGround)
{
	FLORAL_ASSERT(i_r >= i_atmosphere.BottomRadius && i_r <= i_atmosphere.TopRadius);
	FLORAL_ASSERT(i_mu >= -1.0f && i_mu <= 1.0f);
	FLORAL_ASSERT(i_muS >= -1.0f && i_muS <= 1.0f);
	FLORAL_ASSERT(i_nu >= -1.0f && i_nu <= 1.0f);

	// Number of intervals for the numerical integration.
	const s32 k_sampleCount = 50;
	// The integration step, i.e. the length of each integration interval.
	f32 dx = distance_to_nearest_atmosphere_boundary(i_atmosphere, i_r, i_mu, i_rayRMuIntersectsGround) / f32(k_sampleCount);
	// Integration loop.
	floral::vec3f rayleighMieSum(0.0f, 0.0f, 0.0f);
	for (s32 i = 0; i <= k_sampleCount; ++i)
	{
		f32 dI = f32(i) * dx;

		// The i_r, i_mu and i_muS parameters at the current integration point (see the
		// single scattering section for a detailed explanation).
		f32 rI = sqrtf(dI * dI + 2.0 * i_r * i_mu * dI + i_r * i_r);
		rI = floral::clamp(rI, i_atmosphere.BottomRadius, i_atmosphere.TopRadius);

		f32 muI = floral::clamp((i_r * i_mu + dI) / rI, -1.0f, 1.0f);
		f32 muSI = floral::clamp((i_r * i_muS + dI * i_nu) / rI, -1.0f, 1.0f);

		// The Rayleigh and Mie multiple scattering at the current sample point.
		floral::vec3f rayleighMieI =
			get_scattering(i_atmosphere, i_scatteringDensityTexture, rI, muI, muSI, i_nu, i_rayRMuIntersectsGround) *
			get_transmittance(i_atmosphere, i_transmittanceTexture, i_r, i_mu, dI, i_rayRMuIntersectsGround) *
			dx;
		// Sample weight (from the trapezoidal rule).
		f32 weightI = (i == 0 || i == k_sampleCount) ? 0.5f : 1.0f;
		rayleighMieSum += rayleighMieI * weightI;
	}
	return rayleighMieSum;
}

const floral::vec3f compute_multiple_scattering_texture(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture,
	f32** i_scatteringDensityTexture, const floral::vec3f& i_fragCoord, f32* o_nu)
{
	f32 r = 0.0f, mu = 0.0f, muS = 0.0f;
	bool rayRMuIntersectsGround = false;
	get_r_mu_muS_nu_from_scattering_texture_fragcoord(i_atmosphere, i_fragCoord, &r, &mu, &muS, o_nu, &rayRMuIntersectsGround);
	return compute_multiple_scattering(i_atmosphere, i_transmittanceTexture, i_scatteringDensityTexture,
			r, mu, muS, *o_nu, rayRMuIntersectsGround);
}

//-------------------------------------------------------------------

Sky::Sky()
	: m_TexDataArenaRegion { "stone/dynamic/sky", SIZE_MB(128), &m_TexDataArena }
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
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	floral::relative_path wdir = floral::build_relative_path("tests/tech/sky");
	floral::push_directory(m_FileSystem, wdir);

	m_DataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(20));
	g_MemoryManager.initialize_allocator(m_TexDataArenaRegion);

	const f32 k_lengthUnitInMeters = 1000.0f;

	// Values from "Reference Solar Spectral Irradiance: ASTM G-173", ETR column
	// (see http://rredc.nrel.gov/solar/spectra/am1.5/ASTMG173/ASTMG173.html),
	// summed and averaged in each bin (e.g. the value for 360nm is the average
	// of the ASTM G-173 values for all wavelengths between 360 and 370nm).
	// Values in W.m^-2.
	const s32 k_lambdaMin = 360; // nanometers
	const s32 k_lambdaMax = 830; // nanometers
	const f32 k_solarIrradiance[48] = {
		1.11776f, 1.14259f, 1.01249f, 1.14716f, 1.72765f, 1.73054f, 1.6887f, 1.61253f,
		1.91198f, 2.03474f, 2.02042f, 2.02212f, 1.93377f, 1.95809f, 1.91686f, 1.8298f,
		1.8685f, 1.8931f, 1.85149f, 1.8504f, 1.8341f, 1.8345f, 1.8147f, 1.78158f, 1.7533f,
		1.6965f, 1.68194f, 1.64654f, 1.6048f, 1.52143f, 1.55622f, 1.5113f, 1.474f, 1.4482f,
		1.41018f, 1.36775f, 1.34188f, 1.31429f, 1.28303f, 1.26758f, 1.2367f, 1.2082f,
		1.18737f, 1.14683f, 1.12362f, 1.1058f, 1.07124f, 1.04992
	};
	const s32 k_lambdaCount = (k_lambdaMax - k_lambdaMin) / 10 + 1;
	const f32 k_lambdaR = 680.0f;
	const f32 k_lambdaG = 550.0f;
	const f32 k_lambdaB = 440.0f;

	floral::fast_fixed_array<f32, LinearArena> solarIrradiance(m_DataArena);
	solarIrradiance.reserve(k_lambdaCount);
	for (s32 l = k_lambdaMin; l <= k_lambdaMax; l += 10)
	{
		solarIrradiance.push_back(k_solarIrradiance[(l - k_lambdaMin) / 10]);
	}

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
	for (s32 l = k_lambdaMin; l <= k_lambdaMax; l += 10)
	{
		f32 lambda = (f32)l * 1e-3; // convert to micrometers
		f32 mie = k_mieAngstromBeta / k_mieScaleHeight * powf(lambda, -k_mieAngstromAlpha);
		f32 mieScatter = mie * k_mieSingleScatteringAlbedo;
		mieScattering.push_back(mieScatter);
		mieExtinction.push_back(mie);
	}

	// sky model: absorption
	// Values from http://www.iup.uni-bremen.de/gruppen/molspec/databases/
	// referencespectra/o3spectra2011/index.html for 233K, summed and averaged in
	// each bin (e.g. the value for 360nm is the average of the original values
	// for all wavelengths between 360 and 370nm). Values in m^2.
	const f32 k_ozoneCrossSection[48] = {
		1.18e-27f, 2.182e-28f, 2.818e-28f, 6.636e-28f, 1.527e-27f, 2.763e-27f, 5.52e-27f,
		8.451e-27f, 1.582e-26f, 2.316e-26f, 3.669e-26f, 4.924e-26f, 7.752e-26f, 9.016e-26f,
		1.48e-25f, 1.602e-25f, 2.139e-25f, 2.755e-25f, 3.091e-25f, 3.5e-25f, 4.266e-25f,
		4.672e-25f, 4.398e-25f, 4.701e-25f, 5.019e-25f, 4.305e-25f, 3.74e-25f, 3.215e-25f,
		2.662e-25f, 2.238e-25f, 1.852e-25f, 1.473e-25f, 1.209e-25f, 9.423e-26f, 7.455e-26f,
		6.566e-26f, 5.105e-26f, 4.15e-26f, 4.228e-26f, 3.237e-26f, 2.451e-26f, 2.801e-26f,
		2.534e-26f, 1.624e-26f, 1.465e-26f, 2.078e-26f, 1.383e-26f, 7.105e-27f
	};
	// From https://en.wikipedia.org/wiki/Dobson_unit, in molecules.m^-2.
	const f32 k_dobsonUnit = 2.687e20f;
	// Maximum number density of ozone molecules, in m^-3 (computed so at to get
	// 300 Dobson units of ozone - for this we divide 300 DU by the integral of
	// the ozone density profile defined below, which is equal to 15km).
	const f32 k_maxOzoneNumberDensity = 300.0f * k_dobsonUnit / 15000.0f;
	const bool k_useOzone = true;
	floral::fast_fixed_array<f32, LinearArena> absorptionExtinction(m_DataArena);
	absorptionExtinction.reserve(k_lambdaCount);
	for (s32 l = k_lambdaMin; l <= k_lambdaMax; l += 10)
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

	const f32 k_maxSunZenithAngle = floral::to_radians(102.0f);

	// model: initialization
	m_Atmosphere.TopRadius = 6420.0f;
	m_Atmosphere.BottomRadius = 6360.0f;
	m_Atmosphere.MuSMin = cosf(k_maxSunZenithAngle);
	m_Atmosphere.SolarIrradiance = to_rgb(wavelengths, solarIrradiance,
			floral::vec3f(k_lambdaR, k_lambdaG, k_lambdaB), 1.0f);
	m_Atmosphere.SunAngularRadius = 0.004675f;

	m_Atmosphere.RayleighScattering = to_rgb(wavelengths, rayleighScattering,
			floral::vec3f(k_lambdaR, k_lambdaG, k_lambdaB), k_lengthUnitInMeters);
	m_Atmosphere.RayleighDensity.Layers[0] = DensityProfileLayer {
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f
	};
	m_Atmosphere.RayleighDensity.Layers[1] = DensityProfileLayer {
		0.0f / k_lengthUnitInMeters, 1.0f, -1.0f / k_rayleighScaleHeight * k_lengthUnitInMeters, 0.0f * k_lengthUnitInMeters, 0.0f
	};

	m_Atmosphere.MieScattering = to_rgb(wavelengths, mieScattering,
			floral::vec3f(k_lambdaR, k_lambdaG, k_lambdaB), k_lengthUnitInMeters);
	m_Atmosphere.MieExtinction = to_rgb(wavelengths, mieExtinction,
			floral::vec3f(k_lambdaR, k_lambdaG, k_lambdaB), k_lengthUnitInMeters);
	m_Atmosphere.MieDensity.Layers[0] = DensityProfileLayer {
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f
	};
	m_Atmosphere.MieDensity.Layers[1] = DensityProfileLayer {
		0.0f / k_lengthUnitInMeters, 1.0f, -1.0f / k_mieScaleHeight * k_lengthUnitInMeters, 0.0f * k_lengthUnitInMeters, 0.0f
	};
	m_Atmosphere.MiePhaseFunctionG = 0.8f;

	m_Atmosphere.AbsorptionExtinction = to_rgb(wavelengths, absorptionExtinction,
			floral::vec3f(k_lambdaR, k_lambdaG, k_lambdaB), k_lengthUnitInMeters);
	// AbsorptionDensity == OzoneDensity
	// Density profile increasing linearly from 0 to 1 between 10 and 25km, and
	// decreasing linearly from 1 to 0 between 25 and 40km. This is an approximate
	// profile from http://www.kln.ac.lk/science/Chemistry/Teaching_Resources/
	// Documents/Introduction%20to%20atmospheric%20chemistry.pdf (page 10).
	m_Atmosphere.AbsorptionDensity.Layers[0] = DensityProfileLayer {
		25000.0f/ k_lengthUnitInMeters, 0.0f, 0.0f * k_lengthUnitInMeters, 1.0f / 15000.0f * k_lengthUnitInMeters, -2.0f / 3.0f
	};
	m_Atmosphere.AbsorptionDensity.Layers[1] = DensityProfileLayer {
		0.0f / k_lengthUnitInMeters, 0.0f, 0.0f * k_lengthUnitInMeters, -1.0f / 15000.0f * k_lengthUnitInMeters, 8.0f / 3.0f
	};

	m_Atmosphere.GroundAlbedo = floral::vec3f(0.1f, 0.1f, 0.1f);

	BakedDataInfos bakedDataInfos;
	bakedDataInfos.transmittanceTextureWidth = k_transmittanceTextureWidth;
	bakedDataInfos.transmittanceTextureHeight = k_transmittanceTextureHeight;
	bakedDataInfos.scatteringTextureRSize = k_scatteringTextureRSize;
	bakedDataInfos.scatteringTextureMuSize = k_scatteringTextureMuSize;
	bakedDataInfos.scatteringTextureMuSSize = k_scatteringTextureMuSSize;
	bakedDataInfos.scatteringTextureNuSize = k_scatteringTextureNuSize;
	bakedDataInfos.scatteringTextureWidth = k_scatteringTextureWidth;
	bakedDataInfos.scatteringTextureHeight = k_scatteringTextureHeight;
	bakedDataInfos.scatteringTextureDepth = k_scatteringTextureDepth;
	bakedDataInfos.irrandianceTextureWidth = k_irrandianceTextureWidth;
	bakedDataInfos.irrandianceTextureHeight = k_irrandianceTextureHeight;

	SkyFixedConfigs skyFixedConfigs;
	skyFixedConfigs.solarIrradiance = m_Atmosphere.SolarIrradiance;
	skyFixedConfigs.rayleighScattering = m_Atmosphere.RayleighScattering;
	skyFixedConfigs.mieScattering = m_Atmosphere.MieScattering;
	skyFixedConfigs.sunAngularRadius = m_Atmosphere.SunAngularRadius;
	skyFixedConfigs.bottomRadius = m_Atmosphere.BottomRadius;
	skyFixedConfigs.topRadius = m_Atmosphere.TopRadius;
	skyFixedConfigs.miePhaseFunctionG = m_Atmosphere.MiePhaseFunctionG;
	skyFixedConfigs.muSMin = m_Atmosphere.MuSMin;
	skyFixedConfigs.unitLengthInMeters = k_lengthUnitInMeters;

	{
		floral::relative_path oFilePath = floral::build_relative_path("sky.meta");
		floral::file_info oFile = floral::open_file_write(m_FileSystem, oFilePath);
		floral::output_file_stream oStream;
		floral::map_output_file(oFile, &oStream);
		oStream.write(bakedDataInfos);
		oStream.write(skyFixedConfigs);
		floral::close_file(oFile);
	}

	// TODO: note texture directions
	m_TexDataArena.free_all();
	f32* transmittanceTexture = LoadCacheTex2D("transmittance.dat", k_transmittanceTextureWidth, k_transmittanceTextureHeight, 3);
	if (transmittanceTexture == nullptr)
	{
		transmittanceTexture = AllocateTexture2D(k_transmittanceTextureWidth, k_transmittanceTextureHeight, 3);
		for (s32 v = 0; v < k_transmittanceTextureHeight; v++)
		{
			for (s32 u = 0; u < k_transmittanceTextureWidth; u++)
			{
				floral::vec2f fragCoord((f32)u + 0.5f, (f32)v + 0.5f);
				floral::vec3f transmittance = compute_transmittance_to_top_atmosphere_boundary_texture(m_Atmosphere, fragCoord);
				FLORAL_ASSERT(transmittance.x > 0.0f && transmittance.x <= 1.0f);
				FLORAL_ASSERT(transmittance.y > 0.0f && transmittance.y <= 1.0f);
				FLORAL_ASSERT(transmittance.z > 0.0f && transmittance.z <= 1.0f);
				s32 pidx = (v * k_transmittanceTextureWidth + u) * 3;
				transmittanceTexture[pidx] = transmittance.x;
				transmittanceTexture[pidx + 1] = transmittance.y;
				transmittanceTexture[pidx + 2] = transmittance.z;
			}
		}
		WriteCacheTex2D("transmittance.dat", transmittanceTexture, k_transmittanceTextureWidth, k_transmittanceTextureHeight, 3);
	}
	// TODO: check this
	stbi_write_hdr("transmittanceTexture.hdr", k_transmittanceTextureWidth, k_transmittanceTextureHeight, 3, transmittanceTexture);

	// we won't cache this, because we won't write onto it in this section
	f32* irradianceTexture = AllocateTexture2D(k_irrandianceTextureWidth, k_irrandianceTextureHeight, 3);
	f32* deltaIrradianceTexture = LoadCacheTex2D("delta_irradiance.dat", k_irrandianceTextureWidth, k_irrandianceTextureHeight, 3);
	if (deltaIrradianceTexture == nullptr)
	{
		deltaIrradianceTexture = AllocateTexture2D(k_irrandianceTextureWidth, k_irrandianceTextureHeight, 3);
		for (s32 v = 0; v < k_irrandianceTextureHeight; v++)
		{
			for (s32 u = 0; u < k_irrandianceTextureWidth; u++)
			{
				floral::vec2f fragCoord((f32)u + 0.5f, (f32)v + 0.5f);
				floral::vec3f deltaIrradiance = compute_direct_irradiance_texture(m_Atmosphere, transmittanceTexture, fragCoord);

				s32 pidx = (v * k_irrandianceTextureWidth + u) * 3;
				deltaIrradianceTexture[pidx] = deltaIrradiance.x;
				deltaIrradianceTexture[pidx + 1] = deltaIrradiance.y;
				deltaIrradianceTexture[pidx + 2] = deltaIrradiance.z;
			}
		}
		WriteCacheTex2D("delta_irradiance.dat", deltaIrradianceTexture, k_irrandianceTextureWidth, k_irrandianceTextureHeight, 3);
	}
	// TODO: check this
	stbi_write_hdr("deltaIrradianceTexture.hdr", k_irrandianceTextureWidth, k_irrandianceTextureHeight, 3, deltaIrradianceTexture);

	bool needCompute = false;
	f32** deltaRayleighScatteringTexture =
		LoadCacheTex3D("delta_rayleigh_scattering.dat", k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3);
	if (deltaRayleighScatteringTexture == nullptr)
	{
		needCompute = true;
		deltaRayleighScatteringTexture = AllocateTexture3D(k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3);
	}
	f32** deltaMieScatteringTexture =
		LoadCacheTex3D("delta_mie_scattering.dat", k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3);
	if (deltaMieScatteringTexture == nullptr)
	{
		needCompute = true;
		deltaMieScatteringTexture = AllocateTexture3D(k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3);
	}
	f32** scatteringTexture =
		LoadCacheTex3D("scattering.dat", k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 4);
	if (scatteringTexture == nullptr)
	{
		scatteringTexture = AllocateTexture3D(k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 4);
	}

	if (needCompute)
	{
		for (s32 w = 0; w < k_scatteringTextureDepth; w++)
		{
			for (s32 v = 0; v < k_scatteringTextureHeight; v++)
			{
				for (s32 u = 0; u < k_scatteringTextureWidth; u++)
				{
					floral::vec3f fragCoord(((f32)u + 0.5f), ((f32)v + 0.5f), ((f32)w + 0.5f));
					floral::vec3f rayleigh;
					floral::vec3f mie;
					compute_single_scattering_texture(m_Atmosphere, transmittanceTexture, fragCoord, &rayleigh, &mie);

					s32 pidx = (v * k_scatteringTextureWidth + u) * 3;
					deltaRayleighScatteringTexture[w][pidx] = rayleigh.x;
					deltaRayleighScatteringTexture[w][pidx + 1] = rayleigh.y;
					deltaRayleighScatteringTexture[w][pidx + 2] = rayleigh.z;

					deltaMieScatteringTexture[w][pidx] = mie.x;
					deltaMieScatteringTexture[w][pidx + 1] = mie.y;
					deltaMieScatteringTexture[w][pidx + 2] = mie.z;

					s32 pidx2 = (v * k_scatteringTextureWidth + u) * 4;
					scatteringTexture[w][pidx2] = rayleigh.x;
					scatteringTexture[w][pidx2 + 1] = rayleigh.y;
					scatteringTexture[w][pidx2 + 2] = rayleigh.z;
					scatteringTexture[w][pidx2 + 3] = mie.x;
				}
			}
			CLOVER_DEBUG("#%d: done", w);
		}
		WriteCacheTex3D("delta_rayleigh_scattering.dat", deltaRayleighScatteringTexture,
				k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3);
		WriteCacheTex3D("delta_mie_scattering.dat", deltaMieScatteringTexture,
				k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3);
		WriteCacheTex3D("scattering.dat", scatteringTexture,
				k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 4);
	}

	_DebugWriteHDR3D("deltaRayleighScatteringTexture.hdr", k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3, deltaRayleighScatteringTexture);
	_DebugWriteHDR3D("deltaMieScatteringTexture.hdr", k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3, deltaMieScatteringTexture);
	_DebugWriteHDR3D("scatteringTexture.hdr", k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 4, scatteringTexture);
/*
	for (s32 w = 0; w < k_scatteringTextureDepth; w++)
	{
		c8 name[128];
		sprintf(name, "delta_rayleigh_scattering_%02d.hdr", w);
		stbi_write_hdr(name, k_scatteringTextureWidth, k_scatteringTextureHeight, 3, deltaRayleighScatteringTexture[w]);
		sprintf(name, "delta_mie_scattering_%02d.hdr", w);
		stbi_write_hdr(name, k_scatteringTextureWidth, k_scatteringTextureHeight, 3, deltaMieScatteringTexture[w]);
		sprintf(name, "scattering_%02d.hdr", w);
		stbi_write_hdr(name, k_scatteringTextureWidth, k_scatteringTextureHeight, 4, scatteringTexture[w]);
	}
	*/

	f32** deltaScatteringDensityTexture =
		AllocateTexture3D(k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3);
	f32** deltaMultipleScatteringTexture = deltaRayleighScatteringTexture;
		//AllocateTexture3D(k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3);

	for (s32 scatteringOrder = 2; scatteringOrder <= 4; scatteringOrder++)
	{
		// Compute the scattering density, and store it in
		// deltaScatteringDensityTexture
		bool needUpdateCache = false;
		for (s32 w = 0; w < k_scatteringTextureDepth; w++)
		{
			c8 cacheName[256];
			sprintf(cacheName, "ms_order_%d_delta_scattering_density.dat%02d", scatteringOrder, w);
			ssize cacheSize = k_scatteringTextureWidth * k_scatteringTextureHeight * 3 * sizeof(f32);
			voidptr cacheData = LoadCache(cacheName, cacheSize, (voidptr)deltaScatteringDensityTexture[w]);

			if (cacheData == nullptr)
			{
				CLOVER_DEBUG("Cache miss: %s", cacheName);
				needUpdateCache = true;
				for (s32 v = 0; v < k_scatteringTextureHeight; v++)
				{
					for (s32 u = 0; u < k_scatteringTextureWidth; u++)
					{
						floral::vec3f fragCoord(((f32)u + 0.5f), ((f32)v + 0.5f), ((f32)w + 0.5f));

						floral::vec3f scatteringDensity = compute_scattering_density_texture(m_Atmosphere,
								transmittanceTexture, deltaRayleighScatteringTexture, deltaMieScatteringTexture,
								deltaMultipleScatteringTexture, deltaIrradianceTexture, fragCoord, scatteringOrder);

						s32 pidx = (v * k_scatteringTextureWidth + u) * 3;
						deltaScatteringDensityTexture[w][pidx] = scatteringDensity.x;
						deltaScatteringDensityTexture[w][pidx + 1] = scatteringDensity.y;
						deltaScatteringDensityTexture[w][pidx + 2] = scatteringDensity.z;
					}
				}
			}
			else
			{
				CLOVER_DEBUG("Cache hit: %s", cacheName);
			}

			CLOVER_DEBUG("Order %d @ #%d: done", scatteringOrder, w);
/*
			c8 name[128];
			sprintf(name, "ms_order_%d_scattering_density_%02d.hdr", scatteringOrder, w);
			stbi_write_hdr(name, k_scatteringTextureWidth, k_scatteringTextureHeight, 3, deltaScatteringDensityTexture[w]);
			*/
		}

		if (needUpdateCache)
		{
			c8 cacheName[256];
			sprintf(cacheName, "ms_order_%d_delta_scattering_density.dat", scatteringOrder);
			WriteCacheTex3D(cacheName, deltaScatteringDensityTexture, k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3);
		}

		c8 name[128];
		sprintf(name, "ms_order_%d_deltaScatteringDensityTexture.hdr", scatteringOrder);
		_DebugWriteHDR3D(name, k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3, deltaScatteringDensityTexture);

		// Compute the indirect irradiance, store it in deltaIrradianceTexture and
		// accumulate it in irradianceTexture
		for (s32 v = 0; v < k_irrandianceTextureHeight; v++)
		{
			for (s32 u = 0; u < k_irrandianceTextureWidth; u++)
			{
				floral::vec2f fragCoord((f32)u + 0.5f, (f32)v + 0.5f);
				floral::vec3f deltaIrradiance = compute_indirect_irradiance_texture(m_Atmosphere,
						deltaRayleighScatteringTexture, deltaMieScatteringTexture, deltaMultipleScatteringTexture,
						fragCoord, scatteringOrder - 1);
				floral::vec3f irradiance = deltaIrradiance;

				s32 pidx = (v * k_irrandianceTextureWidth + u) * 3;
				deltaIrradianceTexture[pidx] = deltaIrradiance.x;
				deltaIrradianceTexture[pidx + 1] = deltaIrradiance.y;
				deltaIrradianceTexture[pidx + 2] = deltaIrradiance.z;

				irradianceTexture[pidx] += deltaIrradiance.x;
				irradianceTexture[pidx + 1] += deltaIrradiance.y;
				irradianceTexture[pidx + 2] += deltaIrradiance.z;
			}
		}
		sprintf(name, "ms_order_%d_deltaIrradianceTexture.hdr", scatteringOrder);
		stbi_write_hdr(name, k_irrandianceTextureWidth, k_irrandianceTextureHeight, 3, deltaIrradianceTexture);
		sprintf(name, "ms_order_%d_irradianceTexture.hdr", scatteringOrder);
		stbi_write_hdr(name, k_irrandianceTextureWidth, k_irrandianceTextureHeight, 3, irradianceTexture);

		for (s32 w = 0; w < k_scatteringTextureDepth; w++)
		{
			for (s32 v = 0; v < k_scatteringTextureHeight; v++)
			{
				for (s32 u = 0; u < k_scatteringTextureWidth; u++)
				{
					floral::vec3f fragCoord(((f32)u + 0.5f), ((f32)v + 0.5f), ((f32)w + 0.5f));

					f32 nu = 0.0f;
					floral::vec3f deltaMultipleScattering = compute_multiple_scattering_texture(m_Atmosphere,
							transmittanceTexture, deltaScatteringDensityTexture,
							fragCoord, &nu);
					floral::vec4f scattering(0.0f, 0.0f, 0.0f, 0.0f);
					scattering.x = deltaMultipleScattering.x / rayleigh_phase_function(nu);
					scattering.y = deltaMultipleScattering.y / rayleigh_phase_function(nu);
					scattering.z = deltaMultipleScattering.z / rayleigh_phase_function(nu);

					s32 pidx = (v * k_scatteringTextureWidth + u) * 3;
					deltaMultipleScatteringTexture[w][pidx] = deltaMultipleScattering.x;
					deltaMultipleScatteringTexture[w][pidx + 1] = deltaMultipleScattering.y;
					deltaMultipleScatteringTexture[w][pidx + 2] = deltaMultipleScattering.z;

					s32 pidx2 = (v * k_scatteringTextureWidth + u) * 4;
					scatteringTexture[w][pidx2] += scattering.x;
					scatteringTexture[w][pidx2 + 1] += scattering.y;
					scatteringTexture[w][pidx2 + 2] += scattering.z;
					scatteringTexture[w][pidx2 + 3] += scattering.w;
				}
			}

			CLOVER_DEBUG("Order %d @ #%d: done", scatteringOrder, w);
		}

		sprintf(name, "ms_order_%d_deltaRayleighScatteringTexture.hdr", scatteringOrder);
		_DebugWriteHDR3D(name, k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 3, deltaMultipleScatteringTexture);
		sprintf(name, "ms_order_%d_scatteringTexture.hdr", scatteringOrder);
		_DebugWriteHDR3D(name, k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 4, scatteringTexture);
	}

	// now we have to write down
	// transmittanceTexture - 2d
	WriteRawTextureHDR2D("transmittance_texture.rtex2d", k_transmittanceTextureWidth, k_transmittanceTextureHeight, 3, transmittanceTexture);
	// scatteringTexture - 3d
	WriteRawTextureHDR3D("scattering_texture.rtex3d", k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 4, scatteringTexture);
	// irradianceTexture - 2d
	WriteRawTextureHDR2D("irradiance_texture.rtex2d", k_irrandianceTextureWidth, k_irrandianceTextureHeight, 3, irradianceTexture);
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
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);

	g_MemoryManager.destroy_allocator(m_TexDataArenaRegion);
	g_StreammingAllocator.free(m_DataArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------

f32** Sky::AllocateTexture3D(const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel)
{
	ssize sliceSizeBytes = i_w * i_h * i_channel * sizeof(f32);
	f32** tex = (f32**)m_TexDataArena.allocate(i_d * sizeof(f32*));
	for (s32 i = 0; i < i_d; i++)
	{
		tex[i] = (f32*)m_TexDataArena.allocate(sliceSizeBytes);
		memset(tex[i], 0, sliceSizeBytes);
	}
	return tex;
}

f32* Sky::AllocateTexture2D(const s32 i_w, const s32 i_h, const s32 i_channel)
{
	ssize texSizeBytes = i_w * i_h * i_channel * sizeof(f32);
	f32* tex = (f32*)m_TexDataArena.allocate(texSizeBytes);
	memset(tex, 0, texSizeBytes);

	return tex;
}

void Sky::WriteCacheTex2D(const_cstr i_cacheFileName, f32* i_data, const s32 i_w, const s32 i_h, const s32 i_channel)
{
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	WriteCache(i_cacheFileName, i_data, sizeBytes);
}

void Sky::WriteCacheTex3D(const_cstr i_cacheFileName, f32** i_data, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel)
{
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	for (s32 i = 0; i < i_d; i++)
	{
		c8 cacheFileName[256];
		sprintf(cacheFileName, "%s%02d", i_cacheFileName, i);
		WriteCache(cacheFileName, i_data[i], sizeBytes);
	}
}

void Sky::WriteCache(const_cstr i_cacheFileName, voidptr i_data, const ssize i_size)
{
	floral::relative_path oFilePath = floral::build_relative_path(i_cacheFileName);
	floral::file_info oFile = floral::open_file_write(m_FileSystem, oFilePath);
	floral::output_file_stream oStream;
	floral::map_output_file(oFile, &oStream);
	oStream.write_bytes(i_data, i_size);
	floral::close_file(oFile);
}

f32* Sky::LoadCacheTex2D(const_cstr i_cacheFileName, const s32 i_w, const s32 i_h, const s32 i_channel)
{
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	return (f32*)LoadCache(i_cacheFileName, sizeBytes);
}

f32** Sky::LoadCacheTex3D(const_cstr i_cacheFileName, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel)
{
	c8 cacheFileName[256];
	sprintf(cacheFileName, "%s00", i_cacheFileName);

	// try loading the first slice
	floral::relative_path iFilePath = floral::build_relative_path(cacheFileName);
	floral::file_info iFile = floral::open_file_read(m_FileSystem, iFilePath);
	if (iFile.file_size == 0)
	{
		return nullptr;
	}

	f32** data = (f32**)m_TexDataArena.allocate(i_d * sizeof(f32*));
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	for (s32 i = 0; i < i_d; i++)
	{
		sprintf(cacheFileName, "%s%02d", i_cacheFileName, i);
		data[i] = (f32*)LoadCache(cacheFileName, sizeBytes);
	}

	return data;
}

voidptr Sky::LoadCache(const_cstr i_cacheFileName, const ssize i_size, const voidptr i_buffer /* = nullptr */)
{
	floral::relative_path iFilePath = floral::build_relative_path(i_cacheFileName);
	floral::file_info iFile = floral::open_file_read(m_FileSystem, iFilePath);
	if (iFile.file_size == 0)
	{
		return nullptr;
	}
	floral::file_stream iStream;
	FLORAL_ASSERT(iFile.file_size == i_size);
	if (i_buffer == nullptr)
	{
		iStream.buffer = (p8)m_TexDataArena.allocate(iFile.file_size);
	}
	else
	{
		iStream.buffer = (p8)i_buffer;
	}
	floral::read_all_file(iFile, iStream);
	floral::close_file(iFile);
	return (voidptr)iStream.buffer;
}

void Sky::_DebugWriteHDR3D(const_cstr i_fileName, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel, f32** i_data)
{
	m_DataArena->free_all();
	const ssize sliceSizeBytes = i_w * i_h * i_channel * sizeof(f32);

	p8 data = (p8)m_DataArena->allocate(i_d * sliceSizeBytes);
	p8 outData = data;
	for (s32 i = 0; i < i_d; i++)
	{
		memcpy(outData, i_data[i], sliceSizeBytes);
		outData += sliceSizeBytes;
	}

	stbi_write_hdr(i_fileName, i_w, i_h * i_d, i_channel, (f32*)data);
}

void Sky::WriteRawTextureHDR2D(const_cstr i_texFileName, const ssize i_w, const ssize i_h, const s32 i_channel, f32* i_data)
{
	floral::relative_path oFilePath = floral::build_relative_path(i_texFileName);
	floral::file_info oFile = floral::open_file_write(m_FileSystem, oFilePath);
	floral::output_file_stream oStream;
	floral::map_output_file(oFile, &oStream);
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	oStream.write_bytes((voidptr)i_data, sizeBytes);
	floral::close_file(oFile);
}

void Sky::WriteRawTextureHDR3D(const_cstr i_texFileName, const ssize i_w, const ssize i_h, const s32 i_d, const s32 i_channel, f32** i_data)
{
	floral::relative_path oFilePath = floral::build_relative_path(i_texFileName);
	floral::file_info oFile = floral::open_file_write(m_FileSystem, oFilePath);
	floral::output_file_stream oStream;
	floral::map_output_file(oFile, &oStream);
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	for (s32 i = 0; i < i_d; i++)
	{
		oStream.write_bytes((voidptr)i_data[i], sizeBytes);
	}
	floral::close_file(oFile);
}

//-------------------------------------------------------------------
}
}
