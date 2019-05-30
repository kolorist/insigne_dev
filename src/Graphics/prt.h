#pragma once

#include <floral/gpds/vec.h>
#include <floral/stdaliases.h>

namespace stone
{

// high precision data structure
/*
struct highp_vec3_t
{
	f64 x, y, z;
};
*/

typedef floral::vec3<f64> highp_vec3_t;

struct sh_sample
{
	highp_vec3_t sph;
	highp_vec3_t vec;
	f64 coeff[9];
};
//----------------------------------------------

void sh_setup_spherical_samples(sh_sample* samples, s32 sqrt_n_samples);
void sh_project_light_image(f32* imageData, const s32 resolution, const s32 n_samples, const s32 n_coeffs, const sh_sample* samples, highp_vec3_t* result);
void reconstruct_sh_radiance_light_probe(highp_vec3_t* coeffs, f32* imageData, const s32 resolution, const s32 n_samples);
void reconstruct_sh_irradiance_light_probe(highp_vec3_t* coeffs, f32* imageData, const s32 resolution, const s32 n_samples, const f32 exposure = 1.0f, const f32 gamma = 1.0f);

}
