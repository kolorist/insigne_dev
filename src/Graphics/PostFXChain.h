#pragma once

#include <floral/stdaliases.h>
#include <floral/gpds/vec.h>
#include <floral/cmds/crc_string.h>

#include <insigne/commons.h>

#include "MaterialLoader.h"
#include "InsigneHelpers.h"

#include "Memory/MemorySystem.h"

namespace pfx_parser
{
struct PostEffectsDescription;
}

// -------------------------------------------------------------------

namespace pfx_chain
{
// -------------------------------------------------------------------

struct Framebuffer
{
	floral::crc_string							name;
	insigne::framebuffer_handle_t				fbHandle;
	floral::vec2f								dim;
};

struct RenderPass
{
	Framebuffer*								targetFb;
	mat_loader::MaterialShaderPair				msPair;
};

template <class TLinearAllocator>
struct Preset
{
	floral::crc_string										name;
	floral::fast_fixed_array<RenderPass, TLinearAllocator>	renderPasses;
};

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
class PostFXChain
{
public:
	PostFXChain();
	~PostFXChain();

	void										Initialize(const pfx_parser::PostEffectsDescription& i_pfxDesc, const floral::vec2f& i_baseRes, TLinearAllocator* i_memoryArena);
	void										CleanUp();

	void										BeginMainOutput();
	void										EndMainOutput();

	void										Process();

	void										Present();

private:
	Framebuffer*								_FindFramebuffer(const_cstr i_name);
	mat_loader::MaterialShaderPair				_LoadAndCreateMaterial(const_cstr i_fileName);

private:
	helpers::SurfaceGPU							m_SSQuad;

	Framebuffer									m_MainBuffer;
	Framebuffer									m_FinalBuffer;
	floral::fast_fixed_array<Framebuffer, TLinearAllocator>			m_FramebufferList;
	floral::fast_fixed_array<Preset<TLinearAllocator>, TLinearAllocator>	m_Presets;

	mat_loader::MaterialShaderPair*				m_PresentMaterial;

private:
	TLinearAllocator*							m_MemoryArena;
	TFreelistAllocator*							m_TemporalArena;
	TLinearAllocator*							m_MaterialDataArena;
};

// -------------------------------------------------------------------
}

#include "PostFXChain.inl"
