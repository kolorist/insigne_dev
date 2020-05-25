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

enum class ValueType
{
	Invalid = 0,
	Float,
	Vec2,
	Vec3,
	Vec4
};

struct ValueProxy
{
	floral::crc_string							name;
	ValueType									valueType;
	aptr										offset;
};

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
class PostFXChain
{
public:
	PostFXChain();
	~PostFXChain();

	template <class TFileSystem>
	void										Initialize(TFileSystem* i_fs, const pfx_parser::PostEffectsDescription& i_pfxDesc, const floral::vec2f& i_baseRes, TLinearAllocator* i_memoryArena);
	void										CleanUp();
	void										SetValueVec3(const_cstr i_key, const floral::vec3f& i_value);
	void										SetValueVec2(const_cstr i_key, const floral::vec2f& i_value);

	void										BeginMainOutput();
	void										EndMainOutput();

	void										Process();

	void										Present();

private:
	void										_SetValue(const floral::crc_string& i_key, voidptr i_data, const size i_size);
	Framebuffer*								_FindFramebuffer(const_cstr i_name);

	template <class TFileSystem>
	mat_loader::MaterialShaderPair				_LoadAndCreateMaterial(TFileSystem* i_fs, const_cstr i_fileName, aptr& io_offset, voidptr i_data, const insigne::ub_handle_t i_ub);

private:
	helpers::SurfaceGPU							m_SSQuad;

	Framebuffer									m_MainBuffer;
	Framebuffer									m_FinalBuffer;
	floral::fast_fixed_array<Framebuffer, TLinearAllocator>					m_FramebufferList;
	floral::fast_fixed_array<Preset<TLinearAllocator>, TLinearAllocator>	m_Presets;

	mat_loader::MaterialShaderPair*				m_PresentMaterial;

	p8																		m_UBData;
	size																	m_UsedUBBytes;
	insigne::ub_handle_t													m_UB;
	floral::fast_fixed_array<ValueProxy, TLinearAllocator>					m_ValueMap;
	bool																	m_UBDataDirty;

private:
	TLinearAllocator*							m_MemoryArena;
	TFreelistAllocator*							m_TemporalArena;
	TLinearAllocator*							m_MaterialDataArena;
};

// -------------------------------------------------------------------
}

#include "PostFXChain.inl"
