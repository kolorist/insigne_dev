#include "prt.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <random>
#include <algorithm>

namespace stone
{

inline uint64_t factorial(int n)
{
	static const uint64_t factlkp[20] = { 1,1,2,6,24,120,720,5040,
	40320,362880,3628800,39916800,479001600,6227020800,87178291200,
	1307674368000,20922789888000,355687428096000,
	6402373705728000,121645100408832000 };

	return factlkp[n];
}

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<double> dis(0.0, 1.0);

double get_random()
{
	return dis(gen);
}

void map_random_to_spherial_coordinates(double x, double y, double& theta, double& phi)
{
	theta = 2 * acos((sqrt(1.0 - x)));
	phi = 2 * M_PI * y;
}

double legendre(int l, int m, double x)
{
	double pmm = 1.0;
	if (m > 0)
	{
		double somx2 = sqrt((1.0 - x) * (1.0 + x));
		double fact = 1.0;
		for (int i = 1; i <= m; i++)
		{
			pmm *= (-fact) * somx2;
			fact += 2.0;
		}
	}

	if (l == m)
	{
		return pmm;
	}

	double pmmp1 = x * (2.0 * m + 1.0) * pmm;
	if (l == m + 1)
	{
		return pmmp1;
	}

	double pll = 0.0;
	for (int ll = m + 2; ll <= l; ll++)
	{
		pll = ((2.0 * ll - 1.0) * x * pmmp1 - (ll + m - 1.0) * pmm) / (ll - m);
		pmm = pmmp1;
		pmmp1 = pll;
	}

	return pll;
}

double scaling(int l, int m)
{
	double tmp = ((2.0 * l + 1.0) * factorial(l - m)) / (4.0 * M_PI * factorial(l + m));
	return sqrt(tmp);
}

double sh(int l, int m, double theta, double phi)
{
	const double sqrt2 = sqrt(2.0);
	if (m == 0)
	{
		return scaling(l, 0) * legendre(l, m, cos(theta));
	}
	else if (m > 0)
	{
		return sqrt2 * scaling(l, m) * cos(m * phi) * legendre(l, m, cos(theta));
	}
	else
	{
		return sqrt2 * scaling(l, -m) * sin(-m * phi) * legendre(l, -m, cos(theta));
	}
}

// n = N * N

static const int n_bands = 3;

void sh_setup_spherical_samples(sh_sample* samples, int sqrt_n_samples)
{
	int i = 0;
	double oneOverN = 1.0 / sqrt_n_samples;
	for (int a = 0; a < sqrt_n_samples; a++)
	{
		for (int b = 0; b < sqrt_n_samples; b++)
		{
			double x = (a + get_random()) * oneOverN;
			double y = (b + get_random()) * oneOverN;
			double theta = 0, phi = 0;
			map_random_to_spherial_coordinates(x, y, theta, phi);
			samples[i].sph = vec3{ theta, phi, 1.0 };
			//vec3 vec{ sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta) };
			vec3 vec {
				sin(theta) * sin(phi),
				cos(theta),
				sin(theta) * cos(phi)
			};
			samples[i].vec = vec;

			for (int l = 0; l < n_bands; l++)
			{
				for (int m = -l; m <= l; m++)
				{
					int index = l * (l + 1) + m;
					double shval = sh(l, m, theta, phi);
					samples[i].coeff[index] = shval;
				}
			}
			i++;
		}
	}
}

void sh_project_polar_function(sh_polar_fn fn, const int n_samples, const int n_coeffs, const sh_sample* samples, double* result)
{
	const double weight = 4.0 * M_PI;
	for (int i = 0; i < n_samples; i++)
	{
		double theta = samples[i].sph.x;
		double phi = samples[i].sph.y;
		for (int n = 0; n < n_coeffs; n++)
		{
			result[n] += fn(theta, phi) * samples[i].coeff[n];
		}
	}

	double factor = weight / n_samples;
	for (int i = 0; i < n_coeffs; i++)
	{
		result[i] = result[i] * factor;
	}
}

double light_fn(double theta, double phi)
{
	return std::max(0.0, 5.0 * cos(theta) - 4.0) + std::max(0.0, -4.0 * sin(theta - M_PI) * cos(phi - 2.5) - 3.0);
}

void map_cartesian_to_mirror_ball_tex_coord(float x, float y, float z, float& u, float& v)
{
	float d = sqrtf(x * x + y * y);
	float r = (d == 0.0f) ? 0.0f : (1.0f / M_PI / 2.0f) * acosf(z) / d;
	u = 0.5f + x * r;
	v = 0.5f - y * r;
}

void light_probe_access(color3* color, image_t* image, vec3 direction)
{
	float d = sqrt(direction.x * direction.x + direction.y * direction.y);
	float r = (d == 0) ? 0.0f : (1.0f / M_PI / 2.0f) * acos(direction.z) / d;
	float tex_coord[2];
	tex_coord[0] = 0.5f + direction.x * r;
	tex_coord[1] = 0.5f - direction.y * r;
	int pixel_coord[2];
	pixel_coord[0] = tex_coord[0] * image->width;
	pixel_coord[1] = tex_coord[1] * image->height;
	int pixel_index = pixel_coord[1] * image->width + pixel_coord[0];
	color->r = image->hdr_pixels[pixel_index * 3];
	color->g = image->hdr_pixels[pixel_index * 3 + 1];
	color->b = image->hdr_pixels[pixel_index * 3 + 2];
}

void write_to_image(color3 color, double theta, double phi, image_t* image)
{
	vec3 direction{ sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta) };
	float d = sqrt(direction.x * direction.x + direction.y * direction.y);
	float r = (d == 0) ? 0.0f : (1.0f / M_PI / 2.0f) * acos(direction.z) / d;
	float tex_coord[2];
	tex_coord[0] = 0.5f + direction.x * r;
	tex_coord[1] = 0.5f + direction.y * r;
	int pixel_coord[2];
	pixel_coord[0] = tex_coord[0] * image->width;
	pixel_coord[1] = tex_coord[1] * image->height;
	int pixel_index = pixel_coord[1] * image->width + pixel_coord[0];
	image->hdr_pixels[pixel_index * 3] = color.r;
	image->hdr_pixels[pixel_index * 3 + 1] = color.g;
	image->hdr_pixels[pixel_index * 3 + 2] = color.b;
}

void project_to_image(color3 color, double theta, double phi, image_t* image)
{
	vec3 direction{ sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta) };

	if (direction.z < 0) return;

	//float d = sqrt(direction.x * direction.x + direction.y * direction.y);
	//float r = (d == 0) ? 0.0f : (1.0f / M_PI / 2.0f) * acos(direction.z) / d;
	float tex_coord[2];
	tex_coord[0] = 0.5f + direction.x / (1.0 + direction.z) / 2.0;
	tex_coord[1] = 0.5f + direction.y / (1.0 + direction.z) / 2.0;
	//tex_coord[0] = 0.5f + direction.x * r;
	//tex_coord[1] = 0.5f + direction.y * r;
	int pixel_coord[2];
	pixel_coord[0] = tex_coord[0] * image->width;
	pixel_coord[1] = tex_coord[1] * image->height;
	int pixel_index = pixel_coord[1] * image->width + pixel_coord[0];
	image->hdr_pixels[pixel_index * 3] = color.r;
	image->hdr_pixels[pixel_index * 3 + 1] = color.g;
	image->hdr_pixels[pixel_index * 3 + 2] = color.b;
}

void sh_project_light_image(image_t* image, const int n_samples, const int n_coeffs, const sh_sample* samples, color3* result)
{
	const double weight = 4.0 * M_PI;
	for (int i = 0; i < n_samples; i++)
	{
		vec3 direction = samples[i].vec;
		for (int j = 0; j < n_coeffs; j++)
		{
			color3 col;
			light_probe_access(&col, image, direction);
			double shfunc = samples[i].coeff[j];
			result[j].r += (col.r * shfunc);
			result[j].g += (col.g * shfunc);
			result[j].b += (col.b * shfunc);
		}
	}

	double factor = weight / n_samples;
	for (int i = 0; i < n_coeffs; i++)
	{
		result[i].r = result[i].r * factor;
		result[i].g = result[i].g * factor;
		result[i].b = result[i].b * factor;
	}
}
}
#if 0
int main(void)
{
	if (0)
	{
		uint8_t data[512 * 256 * 3];

		memset(data, 0xFF, sizeof(data));

		for (uint32_t i = 0; i < 30000; i++)
		{
			double x = get_random();
			double y = get_random();

			double theta = 0.0;
			double phi = 0.0;
			map_random_to_spherial_coordinates(x, y, theta, phi);

			int32_t imgX = (int32_t)(512 * theta / M_PI);
			int32_t imgY = (int32_t)(256 * phi / (2.0 * M_PI));

			data[(imgY * 512 + imgX) * 3] = 0x00;
			data[(imgY * 512 + imgX) * 3 + 1] = 0x00;
			data[(imgY * 512 + imgX) * 3 + 2] = 0x00;
		}

		stbi_write_bmp("out.bmp", 512, 256, 3, &data);
	}
	
	if (0)
	{
		image_t image;
		image.width = 512;
		image.height = 512;
		image.hdr_pixels = new float[512 * 512 * 3];
		memset(image.hdr_pixels, 0, sizeof(float) * 512 * 512 * 3);

		int samples = 4000;
		for (int i = 0; i < samples; i++)
		{
			for (int j = 0; j < samples; j++)
			{
				double theta = (double)i / samples * M_PI;
				double phi = (double)j / samples * 2.0 * M_PI;

				float c = (float)(light_fn(theta, phi));

				project_to_image(color3{ c, c, c }, theta, phi, &image);
			}
		}

		stbi_write_hdr("out.hdr", image.width, image.height, 3, image.hdr_pixels);

		delete[] image.hdr_pixels;
	}

	if (0)
	{
		int sqrt_n_samples = 100;
		int n_samples = sqrt_n_samples * sqrt_n_samples;
		int n_coeffs = n_bands * n_bands;
		sh_sample* samples = new sh_sample[n_samples];
		for (int i = 0; i < n_samples; i++)
		{
			samples[i].coeff = new double[n_coeffs];
			for (int j = 0; j < n_coeffs; j++)
			{
				samples[i].coeff[j] = 0.0;
			}
		}

		sh_setup_spherical_samples(samples, sqrt_n_samples);
		double* result = new double[n_coeffs];
		for (int i = 0; i < n_coeffs; i++)
		{
			result[i] = 0.0;
		}
		sh_project_polar_function(light_fn, n_samples, n_coeffs, samples, result);
	}

	int channels = 0;
	image_t image;
	image.hdr_pixels = stbi_loadf("grace_probe.hdr", &image.width, &image.height, &channels, 0);

	int sqrt_n_samples = 100;
	int n_samples = sqrt_n_samples * sqrt_n_samples;
	int n_coeffs = n_bands * n_bands;
	sh_sample* samples = new sh_sample[n_samples];
	for (int i = 0; i < n_samples; i++)
	{
		samples[i].coeff = new double[n_coeffs];
		for (int j = 0; j < n_coeffs; j++)
		{
			samples[i].coeff[j] = 0.0;
		}
	}

	sh_setup_spherical_samples(samples, sqrt_n_samples);
	color3* result = new color3[n_coeffs];
	for (int i = 0; i < n_coeffs; i++)
	{
		result[i].r = 0.0f;
		result[i].g = 0.0f;
		result[i].b = 0.0f;
	}
	sh_project_light_image(&image, n_samples, n_coeffs, samples, result);

	return 0;
}
#endif
