#include "prt.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <cstring>

#include <floral/math/rng.h>

namespace stone
{

static const s32								n_bands = 3;
static floral::rng								s_RNG(u64("prt.h"));

inline u64 factorial(u32 n)
{
	static const u64 factlkp[20] = { 1,1,2,6,24,120,720,5040,
	40320,362880,3628800,39916800,479001600,6227020800,87178291200,
	1307674368000,20922789888000,355687428096000,
	6402373705728000,121645100408832000 };

	return factlkp[n];
}

void convert_spherical_to_catersian_coord(f64 theta, f64 phi, highp_vec3_t& vec)
{
	// phi : 0 - M_PI
	// theta : 0 - 2*M_PI

#if 0
	vec.x = sin(theta) * cos(phi);
	vec.y = sin(theta) * sin(phi);
	vec.z = cos(theta);
#else
	vec.z = sin(theta) * cos(phi);
	vec.x = sin(theta) * sin(phi);
	vec.y = cos(theta);
#endif
}

void convert_cartesian_to_lightprobe_coord(highp_vec3_t vec, f64& u, f64& v)
{
	f64 d = sqrt(vec.x * vec.x + vec.y * vec.y);
	f64 r = (d == 0) ? 0.0 : (1.0 / M_PI / 2.0) * acos(vec.z) / d;
	u = 0.5 + vec.x * r;
	v = 0.5 - vec.y * r;
}

void convert_cartesian_to_latlong_coord(highp_vec3_t vec, f64& u, f64& v)
{
	f64 phi = atan2(vec.x, vec.z);
	f64 theta = acos(vec.y);

	u = (floral::pi + phi) * (0.5 / floral::pi);
	v = theta / floral::pi;
}

void convert_cartesian_to_vstrip_cubemap_coord(highp_vec3_t vec, f64& u, f64& v)
{
	f64 absX = abs(vec.x);
	f64 absY = abs(vec.y);
	f64 absZ = abs(vec.z);

	const bool isXPositive = vec.x > 0.0;
	const bool isYPositive = vec.y > 0.0;
	const bool isZPositive = vec.z > 0.0;

	f64 maxAxis = 0.0, uc = 0.0, vc = 0.0;
	u64 faceIndex = 0;

	// positive X
	if (isXPositive && absX >= absY && absX >= absZ)
	{
		// u [0..1] goes from +z to -z
		// v [0..1] goes from -y to +y
		maxAxis = absX;
		uc = -vec.z;
		vc = vec.y;
		faceIndex = 0;
	}
	// negative X
	if (!isXPositive && absX >= absY && absX >= absZ)
	{
		// u [0..1] goes from -z to +z
		// v [0..1] goes from -y to +y
		maxAxis = absX;
		uc = vec.z;
		vc = vec.y;
		faceIndex = 1;
	}
	// positive y
	if (isYPositive && absY >= absX && absY >= absZ)
	{
		// u [0..1] goes from -x to +x
		// v [0..1] goes from +z to -z
		maxAxis = absY;
		uc = vec.x;
		vc = -vec.z;
		faceIndex = 2;
	}
	// negative y
	if (!isYPositive && absY >= absX && absY >= absZ)
	{
		// u [0..1] goes from -x to +x
		// v [0..1] goes from -z to +z
		maxAxis = absY;
		uc = vec.x;
		vc = vec.z;
		faceIndex = 3;
	}
	// positive z
	if (isZPositive && absZ >= absX && absZ >= absY)
	{
		// u [0..1] goes from -x to +x
		// v [0..1] goes from -y to +y
		maxAxis = absZ;
		uc = vec.x;
		vc = vec.y;
		faceIndex = 4;
	}
	// negative z
	if (!isZPositive && absZ >= absX && absZ >= absY)
	{
		// u [0..1] goes from +x to -x
		// v [0..1] goes from -y to +y
		maxAxis = absZ;
		uc = -vec.x;
		vc = vec.y;
		faceIndex = 5;
	}

	uc = 0.5 * (uc / maxAxis + 1.0);
	vc = 0.5 * (vc / maxAxis + 1.0);
	vc = vc / 6.0 + (5 - faceIndex) * 1.0;
	u = uc; v = vc;
}

void convert_cartesian_to_hstrip_cubemap_coord(highp_vec3_t vec, f64& u, f64& v)
{
	f64 absX = abs(vec.x);
	f64 absY = abs(vec.y);
	f64 absZ = abs(vec.z);

	const bool isXPositive = vec.x > 0.0;
	const bool isYPositive = vec.y > 0.0;
	const bool isZPositive = vec.z > 0.0;

	f64 maxAxis = 0.0, uc = 0.0, vc = 0.0;
	u64 faceIndex = 0;

	// positive X
	if (isXPositive && absX >= absY && absX >= absZ)
	{
		// u [0..1] goes from +z to -z
		// v [0..1] goes from -y to +y
		maxAxis = absX;
		uc = -vec.z;
		vc = vec.y;
		faceIndex = 0;
	}
	// negative X
	if (!isXPositive && absX >= absY && absX >= absZ)
	{
		// u [0..1] goes from -z to +z
		// v [0..1] goes from -y to +y
		maxAxis = absX;
		uc = vec.z;
		vc = vec.y;
		faceIndex = 1;
	}
	// positive y
	if (isYPositive && absY >= absX && absY >= absZ)
	{
		// u [0..1] goes from -x to +x
		// v [0..1] goes from +z to -z
		maxAxis = absY;
		uc = vec.x;
		vc = -vec.z;
		faceIndex = 2;
	}
	// negative y
	if (!isYPositive && absY >= absX && absY >= absZ)
	{
		// u [0..1] goes from -x to +x
		// v [0..1] goes from -z to +z
		maxAxis = absY;
		uc = vec.x;
		vc = vec.z;
		faceIndex = 3;
	}
	// positive z
	if (isZPositive && absZ >= absX && absZ >= absY)
	{
		// u [0..1] goes from -x to +x
		// v [0..1] goes from -y to +y
		maxAxis = absZ;
		uc = vec.x;
		vc = vec.y;
		faceIndex = 4;
	}
	// negative z
	if (!isZPositive && absZ >= absX && absZ >= absY)
	{
		// u [0..1] goes from +x to -x
		// v [0..1] goes from -y to +y
		maxAxis = absZ;
		uc = -vec.x;
		vc = vec.y;
		faceIndex = 5;
	}

	uc = 0.5 * (uc / maxAxis + 1.0);
	vc = 0.5 * (vc / maxAxis + 1.0);
	uc = (faceIndex * 1.0 + uc) / 6.0;
	u = uc; v = vc;
}

const bool convert_lightprobe_to_cartesian_coord(const f64 u, const f64 v, highp_vec3_t& vec)
{
	f64 uu = 2.0 * u - 1.0;
	f64 vv = 2.0 * v - 1.0;
	if (uu * uu + vv * vv > 1.0)
	{
		return false;
	}

	f64 tmp = sqrt((u - 0.5) * (u - 0.5) + (0.5 - v) * (0.5 - v));
	vec.z = cos(2.0 * M_PI * tmp);
	if (tmp == 0.0)
	{
		vec.x = 0;
		vec.y = 0;
		return true;
	}

	f64 r = 1.0 / sqrt(1.0 - vec.z * vec.z) * tmp;
	vec.x = (u - 0.5) / r;
	vec.y = (0.5 - v) / r;
	return true;
}

inline void map_uniform_distributions_to_spherical_coord(const f64 x, const f64 y, f64& theta, f64& phi)
{
	theta = 2 * acos((sqrt(1.0 - x)));
	phi = 2 * M_PI * y;
}

f64 legendre(s32 l, s32 m, f64 x)
{
	f64 pmm = 1.0;
	if (m > 0)
	{
		f64 somx2 = sqrt((1.0 - x) * (1.0 + x));
		f64 fact = 1.0;
		for (s32 i = 1; i <= m; i++)
		{
			pmm *= (-fact) * somx2;
			fact += 2.0;
		}
	}

	if (l == m)
	{
		return pmm;
	}

	f64 pmmp1 = x * (2.0 * m + 1.0) * pmm;
	if (l == m + 1)
	{
		return pmmp1;
	}

	f64 pll = 0.0;
	for (s32 ll = m + 2; ll <= l; ll++)
	{
		pll = ((2.0 * ll - 1.0) * x * pmmp1 - (ll + m - 1.0) * pmm) / (ll - m);
		pmm = pmmp1;
		pmmp1 = pll;
	}

	return pll;
}

f64 scaling(s32 l, s32 m)
{
	f64 tmp = ((2.0 * l + 1.0) * factorial(l - m)) / (4.0 * M_PI * factorial(l + m));
	return sqrt(tmp);
}

f64 sh(s32 l, s32 m, f64 theta, f64 phi)
{
	const f64 sqrt2 = sqrt(2.0);
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

void sh_setup_spherical_samples(sh_sample* samples, s32 sqrt_n_samples)
{
	s32 i = 0;
	f64 oneOverN = 1.0 / sqrt_n_samples;
	for (s32 a = 0; a < sqrt_n_samples; a++)
	{
		for (s32 b = 0; b < sqrt_n_samples; b++)
		{
			f64 x = (a + s_RNG.get_f64()) * oneOverN;
			f64 y = (b + s_RNG.get_f64()) * oneOverN;
			f64 theta = 0, phi = 0;
			map_uniform_distributions_to_spherical_coord(x, y, theta, phi);
			samples[i].sph = highp_vec3_t { theta, phi, 1.0 };
			highp_vec3_t vec;
			convert_spherical_to_catersian_coord(theta, phi, vec);
			samples[i].vec = vec;

			for (s32 l = 0; l < n_bands; l++)
			{
				for (s32 m = -l; m <= l; m++)
				{
					s32 index = l * (l + 1) + m;
					f64 shval = sh(l, m, theta, phi);
					samples[i].coeff[index] = shval;
				}
			}
			i++;
		}
	}
}

void light_probe_access(highp_vec3_t* color, f32* imageData, const s32 resolution, highp_vec3_t direction)
{
	f64 tex_coord[2];
	convert_cartesian_to_lightprobe_coord(direction, tex_coord[0], tex_coord[1]);
	s32 pixel_coord[2];
	pixel_coord[0] = (s32)(tex_coord[0] * resolution);
	pixel_coord[1] = (s32)(tex_coord[1] * resolution);
	s32 pixel_index = pixel_coord[1] * resolution + pixel_coord[0];
	color->x = imageData[pixel_index * 3];
	color->y = imageData[pixel_index * 3 + 1];
	color->z = imageData[pixel_index * 3 + 2];
}

void light_probe_access_hstrip(highp_vec3_t* color, f32* imageData, const s32 resolution, highp_vec3_t direction)
{
	f64 tex_coord[2];
	convert_cartesian_to_hstrip_cubemap_coord(direction, tex_coord[0], tex_coord[1]);
	s32 pixel_coord[2];
	pixel_coord[0] = (s32)(tex_coord[0] * resolution * 6);
	pixel_coord[1] = (s32)(tex_coord[1] * resolution);
	s32 pixel_index = pixel_coord[1] * resolution * 6 + pixel_coord[0];
	color->x = imageData[pixel_index * 3];
	color->y = imageData[pixel_index * 3 + 1];
	color->z = imageData[pixel_index * 3 + 2];
}

void light_probe_access_vstrip(highp_vec3_t* color, f32* imageData, const s32 resolution, highp_vec3_t direction)
{
	f64 tex_coord[2];
	convert_cartesian_to_vstrip_cubemap_coord(direction, tex_coord[0], tex_coord[1]);
	s32 pixel_coord[2];
	pixel_coord[0] = (s32)(tex_coord[0] * resolution * 6);
	pixel_coord[1] = (s32)(tex_coord[1] * resolution);
	s32 pixel_index = pixel_coord[1] * resolution * 6 + pixel_coord[0];
	color->x = imageData[pixel_index * 3];
	color->y = imageData[pixel_index * 3 + 1];
	color->z = imageData[pixel_index * 3 + 2];
}

void sh_project_light_image(f32* imageData, const s32 resolution, const s32 n_samples, const s32 n_coeffs, const sh_sample* samples, highp_vec3_t* result)
{
	for (s32 i = 0; i < n_coeffs; i++)
	{
		result[i] = highp_vec3_t { 0.0, 0.0, 0.0 };
	}

	const f64 weight = 4.0 * M_PI;
	for (s32 i = 0; i < n_samples; i++)
	{
		highp_vec3_t direction = samples[i].vec;
		for (s32 j = 0; j < n_coeffs; j++)
		{
			highp_vec3_t col;
			light_probe_access(&col, imageData, resolution, direction);
			//light_probe_access_hstrip(&col, imageData, resolution, direction);
			//light_probe_access_vstrip(&col, imageData, resolution, direction);
			f64 shfunc = samples[i].coeff[j];
			result[j].x += (col.x * shfunc);
			result[j].y += (col.y * shfunc);
			result[j].z += (col.z * shfunc);
		}
	}

	f64 factor = weight / n_samples;
	for (s32 i = 0; i < n_coeffs; i++)
	{
		result[i].x = result[i].x * factor;
		result[i].y = result[i].y * factor;
		result[i].z = result[i].z * factor;
	}
}

void debug_sh_project_light_image(const u32 faceIdx, const s32 resolution, const s32 n_samples, const s32 n_coeffs, const sh_sample* samples, highp_vec3_t* result)
{
	for (s32 i = 0; i < n_coeffs; i++)
	{
		result[i] = highp_vec3_t { 0.0, 0.0, 0.0 };
	}

	highp_vec3_t debugDir(0.0);
	switch (faceIdx)
	{
		case 0: // pos x
			{
				debugDir.x = 1.0;
				break;
			}
		case 1: // neg x
			{
				debugDir.x = -1.0;
				break;
			}
		case 2: // pos y
			{
				debugDir.y = 1.0;
				break;
			}
		case 3: // neg y
			{
				debugDir.y = -1.0;
				break;
			}
		case 4: // pos z
			{
				debugDir.z = 1.0;
				break;
			}
		case 5: // neg z
			{
				debugDir.z = -1.0;
				break;
			}
		default:
			break;
	}

	const f64 weight = 4.0 * M_PI;
	for (s32 i = 0; i < n_samples; i++)
	{
		highp_vec3_t direction = samples[i].vec;
		for (s32 j = 0; j < n_coeffs; j++)
		{
			f64 threshold = cosf(floral::to_radians(30));
			highp_vec3_t col;
			f64 angle = floral::dot(floral::normalize(direction), debugDir);
			if (angle >= threshold)
			{
				col = highp_vec3_t(0.0, 1.0, 0.0);
			}
			else
			{
				col = highp_vec3_t(0.0);
			}

			f64 shfunc = samples[i].coeff[j];
			result[j].x += (col.x * shfunc);
			result[j].y += (col.y * shfunc);
			result[j].z += (col.z * shfunc);
		}
	}

	f64 factor = weight / n_samples;
	for (s32 i = 0; i < n_coeffs; i++)
	{
		result[i].x = result[i].x * factor;
		result[i].y = result[i].y * factor;
		result[i].z = result[i].z * factor;
	}
}

void reconstruct_sh_radiance_light_probe(highp_vec3_t* coeffs, f32* imageData, const s32 resolution, const s32 n_samples)
{
	const f64 c0 = sqrt(1.0 / (4.0 * M_PI));
	const f64 c1 = sqrt(3.0 / (4.0 * M_PI));
	const f64 c2 = sqrt(15.0 / (4.0 * M_PI));
	const f64 c3 = sqrt(5.0 / (16.0 * M_PI));
	const f64 c4 = sqrt(15.0 / (16.0 * M_PI));
	const f64 oneOverN = 1.0 / n_samples;

	for (s32 a = 0; a < n_samples; a++)
	{
		for (s32 b = 0; b < n_samples; b++)
		{
			f64 uu = oneOverN * a;
			f64 vv = oneOverN * b;
			f64 theta = 0, phi = 0;
			highp_vec3_t vec;
			if (convert_lightprobe_to_cartesian_coord(uu, vv, vec))
			{
				highp_vec3_t outColor{ 0.0, 0.0, 0.0 };
				outColor=
					c0 * coeffs[0]

					- c1 * vec.x * coeffs[1]
					+ c1 * vec.y * coeffs[2]
					- c1 * vec.z * coeffs[3]

					+ c2 * vec.z * vec.x * coeffs[4]
					- c2 * vec.x * vec.y * coeffs[5]
					+ c3 * (3.0 * vec.y * vec.y - 1.0) * coeffs[6]
					- c2 * vec.y * vec.z * coeffs[7]
					+ c4 * (vec.z * vec.z - vec.x * vec.x) * coeffs[8];

				f64 u, v;
				convert_cartesian_to_lightprobe_coord(vec, u, v);
				s32 upx = (s32)(u * resolution);
				s32 upy = (s32)(v * resolution);
				s32 pixelIdx = upy * resolution + upx;
				imageData[pixelIdx * 3] = (f32)outColor.x;
				imageData[pixelIdx * 3 + 1] = (f32)outColor.y;
				imageData[pixelIdx * 3 + 2] = (f32)outColor.z;
			}
		}
	}
}

void reconstruct_sh_irradiance_light_probe(highp_vec3_t* coeffs, f32* imageData, const s32 resolution, const s32 n_samples, const f32 exposure, const f32 gamma)
{
	const f64 c0 = sqrt(1.0 / (4.0 * M_PI));
	const f64 c1 = sqrt(3.0 / (4.0 * M_PI));
	const f64 c2 = sqrt(15.0 / (4.0 * M_PI));
	const f64 c3 = sqrt(5.0 / (16.0 * M_PI));
	const f64 c4 = sqrt(15.0 / (16.0 * M_PI));
	const f64 oneOverN = 1.0 / n_samples;

	const f64 a0 = 3.141593;
	const f64 a1 = 2.094395;
	const f64 a2 = 0.785398;

	for (s32 a = 0; a < n_samples; a++)
	{
		for (s32 b = 0; b < n_samples; b++)
		{
			f64 uu = oneOverN * a;
			f64 vv = oneOverN * b;
			f64 theta = 0, phi = 0;
			highp_vec3_t vec;
			if (convert_lightprobe_to_cartesian_coord(uu, vv, vec))
			{
				highp_vec3_t outColor{ 0.0, 0.0, 0.0 };
				outColor =
					a0 * c0 * coeffs[0]

					- a1 * c1 * vec.x * coeffs[1]
					+ a1 * c1 * vec.y * coeffs[2]
					- a1 * c1 * vec.z * coeffs[3]

					+ a2 * c2 * vec.z * vec.x * coeffs[4]
					- a2 * c2 * vec.x * vec.y * coeffs[5]
					+ a2 * c3 * (3.0 * vec.y * vec.y - 1.0) * coeffs[6]
					- a2 * c2 * vec.y * vec.z * coeffs[7]
					+ a2 * c4 * (vec.z * vec.z - vec.x * vec.x) * coeffs[8];

				f64 u, v;
				convert_cartesian_to_lightprobe_coord(vec, u, v);
				s32 upx = (s32)(u * resolution);
				s32 upy = (s32)(v * resolution);
				s32 pixelIdx = upy * resolution + upx;

				highp_vec3_t mappedColor;
				mappedColor.x = 1.0 - exp(-outColor.x * exposure);
				mappedColor.x = pow(mappedColor.x, 1.0 / gamma);
				mappedColor.y = 1.0 - exp(-outColor.y * exposure);
				mappedColor.y = pow(mappedColor.y, 1.0 / gamma);
				mappedColor.z = 1.0 - exp(-outColor.z * exposure);
				mappedColor.z = pow(mappedColor.z, 1.0 / gamma);

				imageData[pixelIdx * 3] = (f32)mappedColor.x;
				imageData[pixelIdx * 3 + 1] = (f32)mappedColor.y;
				imageData[pixelIdx * 3 + 2] = (f32)mappedColor.z;
			}
		}
	}
}

}
