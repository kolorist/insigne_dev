#include "SIMDSamples.h"

namespace stone
{
namespace simd
{
//-------------------------------------------------------------------

f32 calculate_sum_floats(f32* i_arr, const s32 i_len, const s32 i_loopTimes)
{
	f32 sum = 0.0f;
	for (s32 n = 0; n < i_loopTimes; n++)
	{
		sum = 0.0f;
		for (s32 i = 0; i < i_len; i++)
		{
			sum += i_arr[i];
		}
	}
	return sum;
}

//-------------------------------------------------------------------
}
}
