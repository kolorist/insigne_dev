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

//-------------------------------------------------------------------

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

void get_r_mu_from_transmittance_texture_uv(const Atmosphere i_atmosphere, const floral::vec2f& i_fragCoord, f32* o_r, f32* o_mu)
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

void get_r_mu_muS_nu_from_scattering_texture_uvw(const Atmosphere& i_atmosphere, const floral::vec3f& i_fragCoord,
		f32* o_r, f32* o_mu, f32* o_muS, f32* o_nu, bool* o_rayRMuIntersectsGround)
{
	f32 fragCoordNu = floor(i_fragCoord.x / (f32)k_scatteringTextureMuSSize);
	f32 fragCoordMuS = fmod(i_fragCoord.x, (f32)k_scatteringTextureMuSSize);

	floral::vec4f uvwz = (
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

// https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/functions.glsl.html#single_scattering_precomputation
void compute_single_scattering_texture(const Atmosphere& i_atmosphere, const floral::vec3f& i_fragCoord, floral::vec3f* o_rayleigh, floral::vec3f* o_mie)
{
	f32 r = 0.0f, mu = 0.0f, muS = 0.0f, nu = 0.0f;
	bool rayRMuIntersectsGround = false;
	get_r_mu_muS_nu_from_scattering_texture_uvw(i_atmosphere, i_fragCoord, &r, &mu, &muS, &nu, &rayRMuIntersectsGround);
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
	m_Atmosphere.RayleighScattering = to_rgb(wavelengths, rayleighScattering,
			floral::vec3f(k_lambdaR, k_lambdaG, k_lambdaB), k_lengthUnitInMeters);
	m_Atmosphere.RayleighDensity.Layers[0] = DensityProfileLayer {
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f
	};
	m_Atmosphere.RayleighDensity.Layers[1] = DensityProfileLayer {
		0.0f / k_lengthUnitInMeters, 1.0f, -1.0f / k_rayleighScaleHeight * k_lengthUnitInMeters, 0.0f * k_lengthUnitInMeters, 0.0f
	};

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

	{
		// TODO: note texture directions
		m_TemporalArena->free_all();
		ssize bmpSize = k_transmittanceTextureWidth * k_transmittanceTextureHeight * 3;
		p8 bmp = (p8)m_TemporalArena->allocate(bmpSize);
		memset(bmp, 0, bmpSize);
		for (s32 v = 0; v < k_transmittanceTextureHeight; v++)
		{
			for (s32 u = 0; u < k_transmittanceTextureWidth; u++)
			{
				floral::vec2f fragCoord(
						((f32)u + 0.5f) / (f32)k_transmittanceTextureWidth,
						((f32)v + 0.5f) / (f32)k_transmittanceTextureHeight);
				floral::vec3f transmittance = compute_transmittance_to_top_atmosphere_boundary_texture(m_Atmosphere, fragCoord);
				u8 r = (u8)(transmittance.x * 255.0f);
				u8 g = (u8)(transmittance.y * 255.0f);
				u8 b = (u8)(transmittance.z * 255.0f);
				bmp[(v * k_transmittanceTextureWidth + u) * 3] = r;
				bmp[(v * k_transmittanceTextureWidth + u) * 3 + 1] = g;
				bmp[(v * k_transmittanceTextureWidth + u) * 3 + 2] = b;
			}
		}
		// TODO: check this
		stbi_write_tga("out.tga", k_transmittanceTextureWidth, k_transmittanceTextureHeight, 3, (const void*)bmp);
	}

	{
		floral::vec3i texSize(128, 128, 128);
		m_TemporalArena->free_all();

		for (s32 w = 0; w < texSize.z; w++)
		{
			for (s32 u = 0; u < texSize.x; u++)
			{
				for (s32 v = 0; v < texSize.y; v++)
				{
					floral::vec3f fragCoord(((f32)u + 0.5f) / (f32)texSize.x, ((f32)v + 0.5f) / (f32)texSize.y, ((f32)w + 0.5f) / (f32)texSize.z);
					floral::vec3f rayleigh;
					floral::vec3f mie;
					compute_single_scattering_texture(m_Atmosphere, fragCoord, &rayleigh, &mie);
				}
			}
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
