#include "PostFXParser.h"

#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "MaterialParser.h"
#include "SurfaceDefinitions.h"

//#define USE_FULLSCREEN_QUAD

namespace pfx_chain
{
// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
PostFXChain<TLinearAllocator, TFreelistAllocator>::PostFXChain()
	: m_CurrentBuffer(0)
	, m_TAAEnabled(false)
{
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
PostFXChain<TLinearAllocator, TFreelistAllocator>::~PostFXChain()
{
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
template <class TFileSystem>
void PostFXChain<TLinearAllocator, TFreelistAllocator>::Initialize(TFileSystem* i_fs, const pfx_parser::PostEffectsDescription& i_pfxDesc, const floral::vec2f& i_baseRes, TLinearAllocator* i_memoryArena)
{
	m_MemoryArena = i_memoryArena;
	m_MemoryArena->free_all();

	m_TemporalArena = m_MemoryArena->template allocate_arena<TFreelistAllocator>(SIZE_MB(1));
	m_MaterialDataArena = m_MemoryArena->template allocate_arena<TLinearAllocator>(SIZE_KB(128));

	aptr dataOffset = 0;
	m_ValueMap.reserve(64, m_MemoryArena);
	m_UBData = (p8)m_MemoryArena->allocate(SIZE_KB(128));
	insigne::ubdesc_t ubDesc;
	ubDesc.region_size = SIZE_KB(128);
	ubDesc.data = nullptr;
	ubDesc.data_size = SIZE_KB(128);
	ubDesc.usage = insigne::buffer_usage_e::dynamic_draw;
	ubDesc.alignment = 1; // manually align
	m_UB = insigne::create_ub(ubDesc);
	m_UBDataDirty = true;

#ifdef USE_FULLSCREEN_QUAD
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
	m_TAABuffers[0].name = floral::crc_string("_taa0");
	m_TAABuffers[1].name = floral::crc_string("_taa1");
	insigne::framebuffer_desc_t mainDesc = insigne::create_framebuffer_desc();
	mainDesc.width = (s32)i_baseRes.x;
	mainDesc.height = (s32)i_baseRes.y;
	mainDesc.has_depth = true;
	if (i_pfxDesc.mainFbFormat == pfx_parser::ColorFormat::LDR)
	{
		mainDesc.color_attachments->push_back(insigne::color_attachment_t("color0", insigne::texture_format_e::rgba));
	}
	else if (i_pfxDesc.mainFbFormat == pfx_parser::ColorFormat::HDRHigh)
	{
		mainDesc.color_attachments->push_back(insigne::color_attachment_t("color0", insigne::texture_format_e::hdr_rgba));
	}
	else
	{
		mainDesc.color_attachments->push_back(insigne::color_attachment_t("color0", insigne::texture_format_e::hdr_rgb_half));
	}
	m_MainBuffer.fbHandle = insigne::create_framebuffer(mainDesc);
	m_TAABuffers[0].fbHandle = insigne::create_framebuffer(mainDesc);
	m_TAABuffers[1].fbHandle = insigne::create_framebuffer(mainDesc);
	m_MainBuffer.dim = i_baseRes;
	m_TAABuffers[0].dim = i_baseRes;
	m_TAABuffers[1].dim = i_baseRes;

	m_FinalBuffer.name = floral::crc_string("_final");
	m_FinalBuffer.fbHandle = DEFAULT_FRAMEBUFFER_HANDLE;
	m_FinalBuffer.dim = floral::vec2f(0.0f, 0.0f);

	if (i_pfxDesc.fbsCount > 0)
	{
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
	}

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
			newRenderPass.msPair = _LoadAndCreateMaterial(i_fs, passDesc.materialFileName, dataOffset, m_UBData, m_UB);
			// binding
			newRenderPass.mainColorSlot = -1;
			newRenderPass.prevColorSlot = -1;
			for (s32 k = 0; k < passDesc.bindingsCount; k++)
			{
				const pfx_parser::BindDescription& bindDesc = passDesc.bindingList[k];
				if (bindDesc.attachment == pfx_parser::Attachment::Color)
				{
					Framebuffer* sourceFb = _FindFramebuffer(bindDesc.inputFBName);
					// we only support first color attachment of the main color buffer
					if (strcmp(bindDesc.inputFBName, "_main") == 0 && bindDesc.slot == 0)
					{
						newRenderPass.mainColorSlot = insigne::get_material_texture_slot(newRenderPass.msPair.material, bindDesc.samplerName);
					}
					else if (strcmp(bindDesc.inputFBName, "_prev") == 0 && bindDesc.slot == 0)
					{
						newRenderPass.prevColorSlot = insigne::get_material_texture_slot(newRenderPass.msPair.material, bindDesc.samplerName);
					}
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
	m_UsedUBBytes = dataOffset;

	// for taa
	m_TemporalArena->free_all();
	floral::relative_path matPath = floral::build_relative_path("taa.mat");
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(i_fs, matPath, m_TemporalArena);
	const bool matLoadResult = mat_loader::CreateMaterial(&m_TAAMSPair, i_fs, matDesc, m_MemoryArena, m_MaterialDataArena);
	if (matLoadResult)
	{
		m_TAAEnabled = true;
		insigne::helpers::assign_texture(m_TAAMSPair.material, "u_MainTex", insigne::extract_color_attachment(m_MainBuffer.fbHandle, 0));
	}
	else
	{
		m_TAAEnabled = false;
	}

	insigne::begin_render_pass(m_TAABuffers[0].fbHandle);
	insigne::end_render_pass(m_TAABuffers[0].fbHandle);
	insigne::dispatch_render_pass();

	insigne::begin_render_pass(m_TAABuffers[1].fbHandle);
	insigne::end_render_pass(m_TAABuffers[1].fbHandle);
	insigne::dispatch_render_pass();
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
void PostFXChain<TLinearAllocator, TFreelistAllocator>::CleanUp()
{
	m_MemoryArena->free_all();
	m_MaterialDataArena = nullptr;
	m_TemporalArena = nullptr;
	m_MemoryArena = nullptr;
	m_CurrentBuffer = 0;
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
void PostFXChain<TLinearAllocator, TFreelistAllocator>::SetValueVec3(const_cstr i_key, const floral::vec3f& i_value)
{
	floral::crc_string key(i_key);
	for (size i = 0; i < m_ValueMap.get_size(); i++)
	{
		if (m_ValueMap[i].name == key)
		{
			const ValueProxy& valueProxy = m_ValueMap[i];
			FLORAL_ASSERT(valueProxy.valueType == ValueType::Vec3);
			voidptr pData = voidptr((aptr)m_UBData + valueProxy.offset);
			memcpy(pData, (voidptr)&i_value, sizeof(floral::vec3f));
			m_UBDataDirty = true;
		}
	}
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
void PostFXChain<TLinearAllocator, TFreelistAllocator>::SetValueVec2(const_cstr i_key, const floral::vec2f& i_value)
{
	floral::crc_string key(i_key);
	for (size i = 0; i < m_ValueMap.get_size(); i++)
	{
		if (m_ValueMap[i].name == key)
		{
			const ValueProxy& valueProxy = m_ValueMap[i];
			FLORAL_ASSERT(valueProxy.valueType == ValueType::Vec2);
			voidptr pData = voidptr((aptr)m_UBData + valueProxy.offset);
			memcpy(pData, (voidptr)&i_value, sizeof(floral::vec3f));
			m_UBDataDirty = true;
		}
	}
}

// -------------------------------------------------------------------

template <class TLinearAllocator, class TFreelistAllocator>
const bool PostFXChain<TLinearAllocator, TFreelistAllocator>::IsTAAEnabled() const
{
	return m_TAAEnabled;
}

// TODO: we are doing TAA wrong
// - the _main should not be ping-ponged
// - there should be a distinct TAA render pass, which render to a ping-pong framebuffers
// - the ping-pong framebuffer will then be fed to the post-process pass as their normal _main buffer
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
	if (m_UBDataDirty)
	{
		insigne::copy_update_ub(m_UB, m_UBData, m_UsedUBBytes, 0, 1); // alignment == 1 => manually align
		m_UBDataDirty = false;
	}

	// taa
	m_CurrentBuffer = (m_CurrentBuffer + 1) % 2;
	Framebuffer& currFb = m_TAABuffers[m_CurrentBuffer];
	Framebuffer& histFb = m_TAABuffers[(m_CurrentBuffer + 1) % 2];

	if (m_TAAEnabled)
	{
		insigne::begin_render_pass(currFb.fbHandle);
		insigne::helpers::assign_texture(m_TAAMSPair.material, "u_HistTex", insigne::extract_color_attachment(histFb.fbHandle, 0));
		insigne::draw_surface<geo2d::SurfacePT>(m_SSQuad.vb, m_SSQuad.ib, m_TAAMSPair.material);
		insigne::end_render_pass(currFb.fbHandle);
		insigne::dispatch_render_pass();
	}

	const size numRenderPasses = m_Presets[0].renderPasses.get_size() - 1; // exclude final pass
	for (size i = 0; i < numRenderPasses; i++)
	{
		const RenderPass& rp = m_Presets[0].renderPasses[i];
		insigne::begin_render_pass(rp.targetFb->fbHandle);
		mat_loader::MaterialShaderPair& msPair = m_Presets[0].renderPasses[i].msPair;
		if (m_TAAEnabled && rp.mainColorSlot >= 0)
		{
			msPair.material.textures[rp.mainColorSlot].value = insigne::extract_color_attachment(currFb.fbHandle, 0);
		}
		insigne::draw_surface<geo2d::SurfacePT>(m_SSQuad.vb, m_SSQuad.ib, msPair.material);
		insigne::end_render_pass(rp.targetFb->fbHandle);
		insigne::dispatch_render_pass();
	}

	const RenderPass& finalRp = m_Presets[0].renderPasses[numRenderPasses];
	m_PresentMaterial = &(m_Presets[0].renderPasses[numRenderPasses].msPair);
	if (m_TAAEnabled && finalRp.mainColorSlot >= 0)
	{
		m_PresentMaterial->material.textures[finalRp.mainColorSlot].value = insigne::extract_color_attachment(currFb.fbHandle, 0);
	}
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
void PostFXChain<TLinearAllocator, TFreelistAllocator>::_SetValue(const floral::crc_string& i_key, voidptr i_data, const size i_size)
{
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
template <class TFileSystem>
mat_loader::MaterialShaderPair PostFXChain<TLinearAllocator, TFreelistAllocator>::_LoadAndCreateMaterial(TFileSystem* i_fs, const_cstr i_fileName, aptr& io_offset, voidptr i_data, const insigne::ub_handle_t i_ub)
{
	mat_loader::MaterialShaderPair retPair;
	m_TemporalArena->free_all();
	floral::relative_path matPath = floral::build_relative_path(i_fileName);
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(i_fs, matPath, m_TemporalArena);

	// we will create the uniform buffer ourself
	const bool pbrMaterialResult = mat_loader::CreateMaterial<TFileSystem, TLinearAllocator, TFreelistAllocator>(&retPair, i_fs, matDesc, nullptr, nullptr);
	FLORAL_ASSERT(pbrMaterialResult == true);

	FLORAL_ASSERT(matDesc.buffersCount <= 1);
	if (matDesc.buffersCount == 1)
	{
		const mat_parser::UBDescription& ubDesc = matDesc.bufferDescriptions[0];

		static const size k_ParamGPUSize[] =
		{
			0,
			1,		// Int
			1,		// Float
			2,		// Vec2
			4,		// Vec3
			4,		// Vec4
			16,		// Mat3
			16,		// Mat4
			0
		};

		aptr offset = io_offset;
		aptr p = (aptr)i_data;
		for (size i = 0 ; i < ubDesc.membersCount; i++)
		{
			const mat_parser::UBParam& ubParam = ubDesc.members[i];
			const size elemSize = k_ParamGPUSize[(size)ubParam.dataType] * sizeof(f32);
			// fill data
			memcpy(voidptr(p + offset), ubParam.data, elemSize);

			ValueProxy newProxy;
			c8 rawName[256];
			sprintf(rawName, "%s.%s", ubDesc.identifier, ubParam.identifier);
			newProxy.name = floral::crc_string(rawName);
			if (ubParam.dataType == mat_parser::UBParamType::Float)
			{
				newProxy.valueType = ValueType::Float;
			}
			else if (ubParam.dataType == mat_parser::UBParamType::Vec2)
			{
				newProxy.valueType = ValueType::Vec2;
			}
			else if (ubParam.dataType == mat_parser::UBParamType::Vec3)
			{
				newProxy.valueType = ValueType::Vec3;
			}
			else if (ubParam.dataType == mat_parser::UBParamType::Vec4)
			{
				newProxy.valueType = ValueType::Vec4;
			}
			else
			{
				FLORAL_ASSERT(false);
			}
			newProxy.offset = offset;
			m_ValueMap.push_back(newProxy);

			offset += elemSize;
		}
		offset = insigne::helpers::calculate_nearest_ub_offset(offset);
		insigne::helpers::assign_uniform_block(retPair.material, ubDesc.identifier, (size)io_offset, size(offset - io_offset), i_ub);
		io_offset = offset;
	}

	return retPair;
}

// -------------------------------------------------------------------
}
