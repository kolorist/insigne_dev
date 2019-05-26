#pragma once

namespace stone
{

struct vec3
{
	double x, y, z;
};

struct color3
{
	float r, g, b;
};

struct image_t
{
	int width, height;
	float* hdr_pixels;
};

struct sh_sample
{
	vec3 sph;
	vec3 vec;
	double *coeff;
};

typedef double(*sh_polar_fn)(double theta, double phi);

void sh_setup_spherical_samples(sh_sample* samples, int sqrt_n_samples);
void sh_project_polar_function(sh_polar_fn fn, const int n_samples, const int n_coeffs, const sh_sample* samples, double* result);
void sh_project_light_image(image_t* image, const int n_samples, const int n_coeffs, const sh_sample* samples, color3* result);
double light_fn(double theta, double phi);

void map_cartesian_to_mirror_ball_tex_coord(float x, float y, float z, float& u, float& v);

}
