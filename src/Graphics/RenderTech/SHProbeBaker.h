#pragma once

#include <floral.h>

#include <insigne/commons.h>

#include "Memory/MemorySystem.h"

namespace stone
{
namespace tech
{

void											ComputeSH(f64* o_shr, f64* o_shg, f64* o_shb, f32* i_envMap);

struct SHData
{
	floral::vec4f								CoEffs[9];
};

class SHProbeBaker
{
public:
	SHProbeBaker();
	~SHProbeBaker();

	void										Initialize();
	void										CleanUp();
	const bool									FrameUpdate();
	template <class T>
	void										StartSHBaking(const T& i_shPos, voidptr o_shOutput, const floral::simple_callback<void, const floral::mat4x4f&>& i_renderCb);

private:
	floral::fixed_array<floral::vec3f, LinearArena>			m_SHPositions;
	floral::simple_callback<void, const floral::mat4x4f&>	m_RenderCB;
	f32*													m_ProbePixelData;
	SHData*													m_SHOutputBuffer;

	insigne::framebuffer_handle_t							m_ProbeRB;

	size													m_CurrentProbeIdx;
	u64														m_PixelDataReadyFrameIdx;
	bool													m_CurrentProbeCaptured;
	bool													m_IsSHBakingFinished;

private:
	LinearArena*								m_ResourceArena;
};

}
}

#include "SHProbeBaker.inl"
