#include "PostFXParser.h"

#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "MaterialParser.h"
#include "SurfaceDefinitions.h"

namespace pfx_chain
{
// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
PostFXChain<TLinearAllocator, TFreelistAllocator>::PostFXChain()
{
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
PostFXChain<TLinearAllocator, TFreelistAllocator>::~PostFXChain()
{
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
void PostFXChain<TLinearAllocator, TFreelistAllocator>::Initialize(const pfx_parser::PostEffectsDescription& i_pfxDesc, const floral::vec2f& i_baseRes, TLinearAllocator* i_memoryArena)
{
	m_MemoryArena = i_memoryArena;
	m_MemoryArena->free_all();

	m_TemporalArena = m_MemoryArena->template allocate_arena<TFreelistAllocator>(SIZE_MB(1));
	m_MaterialDataArena = m_MemoryArena->template allocate_arena<TLinearAllocator>(SIZE_KB(128));

#if 0
	floral::inplace_array<geo2d::VertexPT, 4> vertices;
	vertices.push_back({ { -1.0f, 1.0f }, { 0.0f, 1.0f } });
	vertices.push_back({ { -1.0f, -1.0f }, { 0.0f, 0.0f } });
	vertices.push_back({ { 1.0f, -1.0f }, { 1.0f, 0.0f } });
	vertices.push_back({ { 1.0f, 1.0f }, { 1.0f, 1.0f } });

	floral::inplace_array<s32, 6> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(0);

	m_SSQuad = helpers::CreateSurfaceGPU(&vertices[0], 4, sizeof(geo2d::VertexPT),
			&indices[0], 6, insigne::buffer_usage_e::static_draw, true);
#else
	floral::inplace_array<geo2d::VertexPT, 3> vertices;
	vertices.push_back({ { -1.0f, 3.0f }, { 0.0f, 2.0f } });
	vertices.push_back({ { -1.0f, -1.0f }, { 0.0f, 0.0f } });
	vertices.push_back({ { 3.0f, -1.0f }, { 2.0f, 0.0f } });

	floral::inplace_array<s32, 3> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);

	m_SSQuad = helpers::CreateSurfaceGPU(&vertices[0], 3, sizeof(geo2d::VertexPT),
			&indices[0], 3, insigne::buffer_usage_e::static_draw, true);
#endif

	m_MainBuffer.name = floral::crc_string("_main");
	insigne::framebuffer_desc_t mainDesc = insigne::create_framebuffer_desc();
	mainDesc.width = (s32)i_baseRes.x;
	mainDesc.height = (s32)i_baseRes.y;
	mainDesc.has_depth = true;
	mainDesc.color_attachments->push_back(insigne::color_attachment_t("color0", insigne::texture_format_e::hdr_rgb_half));
	m_MainBuffer.fbHandle = insigne::create_framebuffer(mainDesc);
	m_MainBuffer.dim = i_baseRes;

	m_FinalBuffer.name = floral::crc_string("_final");
	m_FinalBuffer.fbHandle = DEFAULT_FRAMEBUFFER_HANDLE;
	m_FinalBuffer.dim = floral::vec2f(0.0f, 0.0f);

	m_FramebufferList.reserve(i_pfxDesc.fbsCount, i_memoryArena);
	for (s32 i = 0; i < i_pfxDesc.fbsCount; i++)
	{
		const pfx_parser::FBDescription& fbDesc = i_pfxDesc.fbList[i];

		Framebuffer newFb;
		newFb.name = floral::crc_string(fbDesc.name);
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();

		if (fbDesc.resType == pfx_parser::FBSizeType::Absolute)
		{
			desc.width = (s32)fbDesc.width;
			desc.height = (s32)fbDesc.height;
		}
		else
		{
			desc.width = s32(i_baseRes.x * fbDesc.width);
			desc.height = s32(i_baseRes.y * fbDesc.height);
		}
		newFb.dim = floral::vec2f(desc.width, desc.height);

		for (s32 j = 0; j < fbDesc.colorAttachmentsCount; j++)
		{
			insigne::color_attachment_t atm;
			sprintf(atm.name, "color_%d", j);
			if (fbDesc.colorAttachmentList[j].format == pfx_parser::ColorFormat::LDR)
			{
				atm.texture_format = insigne::texture_format_e::rgb;
			}
			else if (fbDesc.colorAttachmentList[j].format == pfx_parser::ColorFormat::HDRMedium)
			{
				atm.texture_format = insigne::texture_format_e::hdr_rgb_half;
			}
			else
			{
				atm.texture_format = insigne::texture_format_e::hdr_rgb;
			}
			atm.texture_dimension = insigne::texture_dimension_e::tex_2d;
			desc.color_attachments->push_back(atm);
		}

		if (fbDesc.depthFormat == pfx_parser::DepthFormat::On)
		{
			desc.has_depth = true;
		}
		else
		{
			desc.has_depth = false;
		}
		newFb.fbHandle = insigne::create_framebuffer(desc);

		m_FramebufferList.push_back(newFb);
	}

	insigne::dispatch_render_pass();

	m_Presets.reserve(i_pfxDesc.presetsCount, m_MemoryArena);
	m_Presets.resize(i_pfxDesc.presetsCount);
	for (s32 i = 0; i < i_pfxDesc.presetsCount; i++)
	{
		const pfx_parser::PresetDescription& presetDesc = i_pfxDesc.presetList[i];
		Preset<TLinearAllocator>& preset = m_Presets[i];
		preset.name = floral::crc_string(presetDesc.name);
		preset.renderPasses.reserve(presetDesc.passesCount, m_MemoryArena);
		for (s32 j = 0; j < presetDesc.passesCount; j++)
		{
			const pfx_parser::PassDescription& passDesc = presetDesc.passList[j];
			RenderPass newRenderPass;
			newRenderPass.targetFb = _FindFramebuffer(passDesc.targetFBName);
			newRenderPass.msPair = _LoadAndCreateMaterial(passDesc.materialFileName);
			// binding
			for (s32 k = 0; k < passDesc.bindingsCount; k++)
			{
				const pfx_parser::BindDescription& bindDesc = passDesc.bindingList[k];
				if (bindDesc.attachment == pfx_parser::Attachment::Color)
				{
					Framebuffer* sourceFb = _FindFramebuffer(bindDesc.inputFBName);
					insigne::helpers::assign_texture(newRenderPass.msPair.material, bindDesc.samplerName,
							insigne::extract_color_attachment(sourceFb->fbHandle, bindDesc.slot));
				}
				else if (bindDesc.attachment == pfx_parser::Attachment::Depth)
				{
					Framebuffer* sourceFb = _FindFramebuffer(bindDesc.inputFBName);
					insigne::helpers::assign_texture(newRenderPass.msPair.material, bindDesc.samplerName,
							insigne::extract_depth_stencil_attachment(sourceFb->fbHandle));
				}
			}
			preset.renderPasses.push_back(newRenderPass);
		}

		insigne::dispatch_render_pass();
	}
	m_PresentMaterial = nullptr;
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
void PostFXChain<TLinearAllocator, TFreelistAllocator>::CleanUp()
{
	m_MemoryArena->free_all();
	m_MaterialDataArena = nullptr;
	m_TemporalArena = nullptr;
	m_MemoryArena = nullptr;
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
void PostFXChain<TLinearAllocator, TFreelistAllocator>::BeginMainOutput()
{
	insigne::begin_render_pass(m_MainBuffer.fbHandle);
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
void PostFXChain<TLinearAllocator, TFreelistAllocator>::EndMainOutput()
{
	insigne::end_render_pass(m_MainBuffer.fbHandle);
	insigne::dispatch_render_pass();
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
void PostFXChain<TLinearAllocator, TFreelistAllocator>::Process()
{
	const size numRenderPasses = m_Presets[0].renderPasses.get_size() - 1; // exclude final pass
	for (size i = 0;  i < numRenderPasses; i++)
	{
		const RenderPass& rp = m_Presets[0].renderPasses[i];
		insigne::begin_render_pass(rp.targetFb->fbHandle);
		const mat_loader::MaterialShaderPair& msPair = m_Presets[0].renderPasses[i].msPair;
		insigne::draw_surface<geo2d::SurfacePT>(m_SSQuad.vb, m_SSQuad.ib, msPair.material);
		insigne::end_render_pass(rp.targetFb->fbHandle);
		insigne::dispatch_render_pass();
	}

	m_PresentMaterial = &(m_Presets[0].renderPasses[numRenderPasses].msPair);
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
void PostFXChain<TLinearAllocator, TFreelistAllocator>::Present()
{
	insigne::draw_surface<geo2d::SurfacePT>(m_SSQuad.vb, m_SSQuad.ib, m_PresentMaterial->material);
	m_PresentMaterial = nullptr;
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
Framebuffer* PostFXChain<TLinearAllocator, TFreelistAllocator>::_FindFramebuffer(const_cstr i_name)
{
	if (strcmp(i_name, "_main") == 0)
	{
		return &m_MainBuffer;
	}
	else if (strcmp(i_name, "_final") == 0)
	{
		return &m_FinalBuffer;
	}

	floral::crc_string keyToFind(i_name);
	for (size i = 0; i < m_FramebufferList.get_size(); i++)
	{
		if (m_FramebufferList[i].name == keyToFind)
		{
			return &m_FramebufferList[i];
		}
	}

	FLORAL_ASSERT(false);
	return nullptr;
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
mat_loader::MaterialShaderPair PostFXChain<TLinearAllocator, TFreelistAllocator>::_LoadAndCreateMaterial(const_cstr i_fileName)
{
	mat_loader::MaterialShaderPair retPair;
	m_TemporalArena->free_all();
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(floral::path(i_fileName), m_TemporalArena);

	const bool pbrMaterialResult = mat_loader::CreateMaterial(&retPair, matDesc, m_MaterialDataArena);
	FLORAL_ASSERT(pbrMaterialResult == true);
	return retPair;
}

// -------------------------------------------------------------------
}
