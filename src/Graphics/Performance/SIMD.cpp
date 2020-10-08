#include "SIMD.h"

#include <chrono>

#include <clover/Logger.h>
#include <insigne/ut_render.h>


#include "InsigneImGui.h"
#include "FastMath/SIMDSamples.h"

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------
static constexpr s32 k_ArraySize = 1000;
static constexpr s32 k_LoopTimes = 10000;

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

SIMD::SIMD()
{
}

SIMD::~SIMD()
{
}

ICameraMotion* SIMD::GetCameraMotion()
{
	return nullptr;
}

const_cstr SIMD::GetName() const
{
	return k_name;
}

void SIMD::_OnInitialize()
{
	m_MemoryArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));

	m_FloatArray = m_MemoryArena->allocate_array<f32>(k_ArraySize);
	srand(1234);
	for (int i = 0; i < k_ArraySize; i++)
	{
		f32 r = 5.0f * (f32)rand() / RAND_MAX;
		m_FloatArray[i] = r;
	}
}

void SIMD::_OnUpdate(const f32 i_deltaMs)
{
	using namespace std::chrono;

	high_resolution_clock::time_point start = high_resolution_clock::now();
	f32 sumSimd = simd::calculate_sum_floats(m_FloatArray, k_ArraySize, k_LoopTimes);
	high_resolution_clock::time_point end = high_resolution_clock::now();
	duration<f64, std::milli> durSimd = end - start;

	start = high_resolution_clock::now();
	f32 sumNoSimd = calculate_sum_floats(m_FloatArray, k_ArraySize, k_LoopTimes);
	end = high_resolution_clock::now();
	duration<f64, std::milli> durNoSimd = end - start;

	ImGui::Begin("Controller##SIMD");
	ImGui::Text("Sum (simd): %f", sumSimd);
	ImGui::Text("Time (simd): %f ms", durSimd.count());
	ImGui::Text("Sum (no simd): %f", sumNoSimd);
	ImGui::Text("Time (no simd): %f ms", durNoSimd.count());
	ImGui::End();
}

void SIMD::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void SIMD::_OnCleanUp()
{
	g_StreammingAllocator.free(m_MemoryArena);
}

//-------------------------------------------------------------------
}
}
