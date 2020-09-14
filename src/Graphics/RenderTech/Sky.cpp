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
	FLORAL_ASSERT(i_mu >= 1.0f && i_mu <= 1.0f);

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

void get_r_mu_from_transmittance_texture_uv(const Atmosphere i_atmosphere, const floral::vec2f& i_fragCoord,
		const floral::vec2i& i_texSize, f32* o_r, f32* o_mu)
{
	FLORAL_ASSERT(i_fragCoord.x >= 0.0f && i_fragCoord.x <= 1.0f);
	FLORAL_ASSERT(i_fragCoord.y >= 0.0f && i_fragCoord.y <= 1.0f);

	f32 adjustedMu = get_unit_range_from_texture_coord(i_fragCoord.x, i_texSize.x);
	f32 adjustedR = get_unit_range_from_texture_coord(i_fragCoord.y, i_texSize.y);

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
	FLORAL_ASSERT(i_mu >= 1.0f && i_mu <= 1.0f);

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
	FLORAL_ASSERT(i_mu >= 1.0f && i_mu <= 1.0f);
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
floral::vec3f compute_transmittance_to_top_atmosphere_boundary_texture(const Atmosphere& i_atmosphere, const floral::vec2f& i_fragCoord, const floral::vec2i& i_texSize)
{
	f32 r = 0.0f, mu = 0.0f;
	get_r_mu_from_transmittance_texture_uv(i_atmosphere, i_fragCoord, i_texSize, &r, &mu);
	return compute_transmittance_to_top_atmosphere_boundary(i_atmosphere, r, mu);
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
	const f32 k_rayleigh = 1.24062e-6;
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
	const f32 k_mieAngstromBeta = 5.328e-3;
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

	// model: initialization
	// TODO: check this
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

	floral::vec2i texSize(128, 128);
	p8 bmp = (p8)m_TemporalArena->allocate(texSize.x * texSize.y * 3);
	memset(bmp, 0, texSize.x * texSize.y * 3);
	for (s32 u = 0; u < texSize.x; u++)
	{
		for (s32 v = 0; v < texSize.y; v++)
		{
			floral::vec2f fragCoord((f32)u / 127.0f, (f32)v / 127.0f);
			floral::vec3f transmittance = compute_transmittance_to_top_atmosphere_boundary_texture(m_Atmosphere, fragCoord, texSize);
			u8 r = (u8)(transmittance.x * 255.0f);
			u8 g = (u8)(transmittance.y * 255.0f);
			u8 b = (u8)(transmittance.z * 255.0f);
			bmp[(u * texSize.x + v) * 3] = r;
			bmp[(u * texSize.x + v) * 3 + 1] = g;
			bmp[(u * texSize.x + v) * 3 + 2] = b;
		}
	}
	stbi_write_tga("out.tga", texSize.x, texSize.y, 3, (const void*)bmp);
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
