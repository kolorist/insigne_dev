#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "TextureLoader.h"

namespace mat_loader
{
// ----------------------------------------------------------------------------

template <class TMemoryAllocator>
const bool CreateMaterial(MaterialShaderPair* o_mat, const mat_parser::MaterialDescription& i_matDesc, TMemoryAllocator* i_dataAllocator)
{
	// create shader
	insigne::shader_desc_t shaderDesc = insigne::create_shader_desc();
	for (size i = 0; i < i_matDesc.buffersCount; i++)
	{
		shaderDesc.reflection.uniform_blocks->push_back(
				insigne::shader_param_t(i_matDesc.bufferDescriptions[i].identifier, insigne::param_data_type_e::param_ub));
	}

	for (size i = 0; i < i_matDesc.texturesCount; i++)
	{
		const mat_parser::TextureDescription& texDesc = i_matDesc.textureDescriptions[i];
		switch (texDesc.dimension)
		{
		case mat_parser::TextureDimension::Texture2D:
			shaderDesc.reflection.textures->push_back(
					insigne::shader_param_t(texDesc.identifier, insigne::param_data_type_e::param_sampler2d));
			break;
		case mat_parser::TextureDimension::TextureCube:
			shaderDesc.reflection.textures->push_back(
					insigne::shader_param_t(texDesc.identifier, insigne::param_data_type_e::param_sampler_cube));
			break;
		default:
			break;
		}
	}

	shaderDesc.vs_path = floral::path(i_matDesc.vertexShaderPath);
	floral::file_info inp = floral::open_file(i_matDesc.vertexShaderPath);
	floral::read_all_file(inp, shaderDesc.vs);
	shaderDesc.vs[inp.file_size] = 0;
	floral::close_file(inp);

	shaderDesc.fs_path = floral::path(i_matDesc.fragmentShaderPath);
	inp = floral::open_file(i_matDesc.fragmentShaderPath);
	floral::read_all_file(inp, shaderDesc.fs);
	shaderDesc.fs[inp.file_size] = 0;
	floral::close_file(inp);

	o_mat->shader = insigne::create_shader(shaderDesc);
	insigne::infuse_material(o_mat->shader, o_mat->material);

	// build uniform buffer
	for (size i = 0; i < i_matDesc.buffersCount; i++)
	{
		const insigne::ub_handle_t ub = internal::BuildUniformBuffer(i_matDesc.bufferDescriptions[i], i_dataAllocator);
		insigne::helpers::assign_uniform_block(o_mat->material,
				i_matDesc.bufferDescriptions[i].identifier, 0, 0, ub);
	}

	// build textures
	for (size i = 0; i < i_matDesc.texturesCount; i++)
	{
		const mat_parser::TextureDescription& desc = i_matDesc.textureDescriptions[i];
		if (!desc.isPlaceholder)
		{
			insigne::texture_desc_t texDesc;
			switch (desc.minFilter)
			{
			case mat_parser::TextureFilter::Nearest:
				texDesc.min_filter = insigne::filtering_e::nearest;
				break;
			case mat_parser::TextureFilter::Linear:
				texDesc.min_filter = insigne::filtering_e::linear;
				break;
			case mat_parser::TextureFilter::NearestMipmapNearest:
				texDesc.min_filter = insigne::filtering_e::nearest_mipmap_nearest;
				break;
			case mat_parser::TextureFilter::LinearMipmapNearest:
				texDesc.min_filter = insigne::filtering_e::linear_mipmap_nearest;
				break;
			case mat_parser::TextureFilter::NearestMipmapLinear:
				texDesc.min_filter = insigne::filtering_e::nearest_mipmap_linear;
				break;
			case mat_parser::TextureFilter::LinearMipmapLinear:
				texDesc.min_filter = insigne::filtering_e::linear_mipmap_linear;
				break;
			default:
				break;
			}

			switch (desc.magFilter)
			{
			case mat_parser::TextureFilter::Nearest:
				texDesc.mag_filter = insigne::filtering_e::nearest;
				break;
			case mat_parser::TextureFilter::Linear:
				texDesc.mag_filter = insigne::filtering_e::linear;
				break;
			default:
				break;
			}

			switch (desc.dimension)
			{
			case mat_parser::TextureDimension::Texture2D:
			{
				insigne::texture_handle_t tex = tex_loader::LoadLDRTexture2D(floral::path(desc.texturePath), texDesc, true);
				insigne::helpers::assign_texture(o_mat->material, desc.identifier, tex);
				break;
			}
			default:
				break;
			}
		}
	}

	return true;
}

namespace internal
{
// ----------------------------------------------------------------------------

template <class TMemoryAllocator>
const insigne::ub_handle_t BuildUniformBuffer(const mat_parser::UBDescription& i_ubDesc, TMemoryAllocator* i_dataAllocator)
{
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

	// pre-allocate
	size dataSize = 0;
	for (size i = 0 ; i < i_ubDesc.membersCount; i++)
	{
		dataSize += k_ParamGPUSize[(size)i_ubDesc.members[i].dataType] * sizeof(f32);
	}
	p8 data = (p8)i_dataAllocator->allocate(dataSize);
	memset(data, 0, dataSize);

	// fill data
	aptr p = (aptr)data;
	for (size i = 0; i < i_ubDesc.membersCount; i++)
	{
		const mat_parser::UBParam& ubParam = i_ubDesc.members[i];
		const size elemSize = k_ParamGPUSize[(size)ubParam.dataType] * sizeof(f32);
		memcpy((voidptr)p, ubParam.data, elemSize);
		p += elemSize;
	}

	// upload to gpu
	insigne::ubdesc_t desc;
	desc.region_size = dataSize;
	desc.data = data;
	desc.data_size = dataSize;
	desc.usage = insigne::buffer_usage_e::static_draw;

	// we are not supposed to change the data here, so, no need to copy_create_ub
	return insigne::create_ub(desc);
}

// ----------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
}
