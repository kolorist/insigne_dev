#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include <floral/io/filesystem.h>

#include "TextureLoader.h"

namespace mat_loader
{
// ----------------------------------------------------------------------------

inline insigne::wrap_e ToInsigneWrap(const mat_parser::TextureWrap i_value)
{
	switch (i_value)
	{
	case mat_parser::TextureWrap::ClampToEdge:
		return insigne::wrap_e::clamp_to_edge;
	case mat_parser::TextureWrap::MirroredRepeat:
		return insigne::wrap_e::mirrored_repeat;
	case mat_parser::TextureWrap::Repeat:
		return insigne::wrap_e::repeat;
	default:
		FLORAL_ASSERT(false);
		break;
	}
	return insigne::wrap_e::clamp_to_edge;
}

template <class TIOAllocator, class TMemoryAllocator>
const bool CreateMaterial(MaterialShaderPair* o_mat, const mat_parser::MaterialDescription& i_matDesc, TIOAllocator* i_ioAllocator, TMemoryAllocator* i_dataAllocator)
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

	// render state
	insigne::render_state_t& renderState = o_mat->material.render_state;
	const u8 k_toggle[] = {
		1,
		0,
		2
	};
	const insigne::compare_func_e k_compareFunc[] = {
		insigne::compare_func_e::func_never,
		insigne::compare_func_e::func_less,
		insigne::compare_func_e::func_equal,
		insigne::compare_func_e::func_less_or_equal,
		insigne::compare_func_e::func_greater,
		insigne::compare_func_e::func_not_equal,
		insigne::compare_func_e::func_greater_or_equal,
		insigne::compare_func_e::func_always,
		insigne::compare_func_e::func_undefined
	};
	renderState.depth_write = k_toggle[s32(i_matDesc.renderState.depthWrite)];
	renderState.depth_test = k_toggle[s32(i_matDesc.renderState.depthTest)];
	renderState.depth_func = k_compareFunc[s32(i_matDesc.renderState.depthFunc)];

	const insigne::face_side_e k_faceSide[] = {
		insigne::face_side_e::front_side,
		insigne::face_side_e::back_side,
		insigne::face_side_e::front_and_back_side,
		insigne::face_side_e::undefined_side
	};
	const insigne::front_face_e k_frontFace[] = {
		insigne::front_face_e::face_cw,
		insigne::front_face_e::face_ccw,
		insigne::front_face_e::face_undefined
	};
	renderState.cull_face = k_toggle[s32(i_matDesc.renderState.cullFace)];
	renderState.face_side = k_faceSide[s32(i_matDesc.renderState.faceSide)];
	renderState.front_face = k_frontFace[s32(i_matDesc.renderState.frontFace)];

	const insigne::blend_equation_e k_blendEquation[] = {
		insigne::blend_equation_e::func_add,
		insigne::blend_equation_e::func_substract,
		insigne::blend_equation_e::func_reverse_substract,
		insigne::blend_equation_e::func_min,
		insigne::blend_equation_e::func_max,
		insigne::blend_equation_e::func_undefined
	};
	const insigne::factor_e k_blendFactor[] = {
		insigne::factor_e::fact_zero,
		insigne::factor_e::fact_one,
		insigne::factor_e::fact_src_color,
		insigne::factor_e::fact_one_minus_src_color,
		insigne::factor_e::fact_dst_color,
		insigne::factor_e::fact_one_minus_dst_color,
		insigne::factor_e::fact_src_alpha,
		insigne::factor_e::fact_one_minus_src_alpha,
		insigne::factor_e::fact_dst_alpha,
		insigne::factor_e::fact_one_minus_dst_alpha,
		insigne::factor_e::fact_constant_color,
		insigne::factor_e::fact_one_minus_constant_color,
		insigne::factor_e::fact_constant_alpha,
		insigne::factor_e::fact_one_minus_constant_alpha,
		insigne::factor_e::fact_undefined
	};
	renderState.blending = k_toggle[s32(i_matDesc.renderState.blending)];
	renderState.blend_equation = k_blendEquation[s32(i_matDesc.renderState.blendEquation)];
	renderState.blend_func_sfactor = k_blendFactor[s32(i_matDesc.renderState.blendSourceFactor)];
	renderState.blend_func_dfactor = k_blendFactor[s32(i_matDesc.renderState.blendDestinationFactor)];

	if (i_dataAllocator)
	{
		// build uniform buffer
		for (size i = 0; i < i_matDesc.buffersCount; i++)
		{
			const insigne::ub_handle_t ub = internal::BuildUniformBuffer(i_matDesc.bufferDescriptions[i], i_dataAllocator);
			insigne::helpers::assign_uniform_block(o_mat->material,
					i_matDesc.bufferDescriptions[i].identifier, 0, 0, ub);
		}
	}

	// build textures
	for (size i = 0; i < i_matDesc.texturesCount; i++)
	{
		const mat_parser::TextureDescription& desc = i_matDesc.textureDescriptions[i];
		if (!desc.isPlaceholder)
		{
			FLORAL_ASSERT(i_ioAllocator != nullptr);
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
			case mat_parser::TextureDimension::TextureCube:
			{
				texDesc.wrap_s = ToInsigneWrap(desc.wrapS);
				texDesc.wrap_t = ToInsigneWrap(desc.wrapT);
				texDesc.wrap_r = ToInsigneWrap(desc.wrapR);
				break;
			}
			case mat_parser::TextureDimension::Texture2D:
			{
				texDesc.wrap_s = ToInsigneWrap(desc.wrapS);
				texDesc.wrap_t = ToInsigneWrap(desc.wrapT);
				break;
			}
			default:
				break;
			}

			insigne::texture_handle_t tex = tex_loader::LoadCBTexture(floral::path(desc.texturePath), texDesc, i_ioAllocator, true);
			insigne::dispatch_render_pass();
			insigne::helpers::assign_texture(o_mat->material, desc.identifier, tex);
		}
	}

	return true;
}

template <class TMemoryAllocator>
const bool CreateMaterial(MaterialShaderPair* o_mat, const mat_parser::MaterialDescription& i_matDesc, TMemoryAllocator* i_dataAllocator)
{
	return CreateMaterial(o_mat, i_matDesc, (TMemoryAllocator*)nullptr, i_dataAllocator);
}

template <class TFileSystem, class TIOAllocator, class TMemoryAllocator>
const bool CreateMaterial(MaterialShaderPair* o_mat, TFileSystem* i_fs, const mat_parser::MaterialDescription& i_matDesc, TIOAllocator* i_ioAllocator, TMemoryAllocator* i_dataAllocator)
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
	floral::relative_path vsPath = floral::build_relative_path(i_matDesc.vertexShaderPath);
	floral::file_info inp = floral::open_file_read(i_fs, vsPath);
	floral::read_all_file(inp, shaderDesc.vs);
	shaderDesc.vs[inp.file_size] = 0;
	floral::close_file(inp);

	shaderDesc.fs_path = floral::path(i_matDesc.fragmentShaderPath);
	floral::relative_path fsPath = floral::build_relative_path(i_matDesc.fragmentShaderPath);
	inp = floral::open_file_read(i_fs, fsPath);
	floral::read_all_file(inp, shaderDesc.fs);
	shaderDesc.fs[inp.file_size] = 0;
	floral::close_file(inp);

	o_mat->shader = insigne::create_shader(shaderDesc);
	insigne::infuse_material(o_mat->shader, o_mat->material);

	// render state
	insigne::render_state_t& renderState = o_mat->material.render_state;
	const u8 k_toggle[] = {
		1,
		0,
		2
	};
	const insigne::compare_func_e k_compareFunc[] = {
		insigne::compare_func_e::func_never,
		insigne::compare_func_e::func_less,
		insigne::compare_func_e::func_equal,
		insigne::compare_func_e::func_less_or_equal,
		insigne::compare_func_e::func_greater,
		insigne::compare_func_e::func_not_equal,
		insigne::compare_func_e::func_greater_or_equal,
		insigne::compare_func_e::func_always,
		insigne::compare_func_e::func_undefined
	};
	renderState.depth_write = k_toggle[s32(i_matDesc.renderState.depthWrite)];
	renderState.depth_test = k_toggle[s32(i_matDesc.renderState.depthTest)];
	renderState.depth_func = k_compareFunc[s32(i_matDesc.renderState.depthFunc)];

	const insigne::face_side_e k_faceSide[] = {
		insigne::face_side_e::front_side,
		insigne::face_side_e::back_side,
		insigne::face_side_e::front_and_back_side,
		insigne::face_side_e::undefined_side
	};
	const insigne::front_face_e k_frontFace[] = {
		insigne::front_face_e::face_cw,
		insigne::front_face_e::face_ccw,
		insigne::front_face_e::face_undefined
	};
	renderState.cull_face = k_toggle[s32(i_matDesc.renderState.cullFace)];
	renderState.face_side = k_faceSide[s32(i_matDesc.renderState.faceSide)];
	renderState.front_face = k_frontFace[s32(i_matDesc.renderState.frontFace)];

	const insigne::blend_equation_e k_blendEquation[] = {
		insigne::blend_equation_e::func_add,
		insigne::blend_equation_e::func_substract,
		insigne::blend_equation_e::func_reverse_substract,
		insigne::blend_equation_e::func_min,
		insigne::blend_equation_e::func_max,
		insigne::blend_equation_e::func_undefined
	};
	const insigne::factor_e k_blendFactor[] = {
		insigne::factor_e::fact_zero,
		insigne::factor_e::fact_one,
		insigne::factor_e::fact_src_color,
		insigne::factor_e::fact_one_minus_src_color,
		insigne::factor_e::fact_dst_color,
		insigne::factor_e::fact_one_minus_dst_color,
		insigne::factor_e::fact_src_alpha,
		insigne::factor_e::fact_one_minus_src_alpha,
		insigne::factor_e::fact_dst_alpha,
		insigne::factor_e::fact_one_minus_dst_alpha,
		insigne::factor_e::fact_constant_color,
		insigne::factor_e::fact_one_minus_constant_color,
		insigne::factor_e::fact_constant_alpha,
		insigne::factor_e::fact_one_minus_constant_alpha,
		insigne::factor_e::fact_undefined
	};
	renderState.blending = k_toggle[s32(i_matDesc.renderState.blending)];
	renderState.blend_equation = k_blendEquation[s32(i_matDesc.renderState.blendEquation)];
	renderState.blend_func_sfactor = k_blendFactor[s32(i_matDesc.renderState.blendSourceFactor)];
	renderState.blend_func_dfactor = k_blendFactor[s32(i_matDesc.renderState.blendDestinationFactor)];

	if (i_dataAllocator)
	{
		// build uniform buffer
		for (size i = 0; i < i_matDesc.buffersCount; i++)
		{
			const insigne::ub_handle_t ub = internal::BuildUniformBuffer(i_matDesc.bufferDescriptions[i], i_dataAllocator);
			insigne::helpers::assign_uniform_block(o_mat->material,
					i_matDesc.bufferDescriptions[i].identifier, 0, 0, ub);
		}
	}

	// build textures
	for (size i = 0; i < i_matDesc.texturesCount; i++)
	{
		const mat_parser::TextureDescription& desc = i_matDesc.textureDescriptions[i];
		if (!desc.isPlaceholder)
		{
			FLORAL_ASSERT(i_ioAllocator != nullptr);
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
			case mat_parser::TextureDimension::TextureCube:
			{
				texDesc.wrap_s = ToInsigneWrap(desc.wrapS);
				texDesc.wrap_t = ToInsigneWrap(desc.wrapT);
				texDesc.wrap_r = ToInsigneWrap(desc.wrapR);
				break;
			}
			case mat_parser::TextureDimension::Texture2D:
			{
				texDesc.wrap_s = ToInsigneWrap(desc.wrapS);
				texDesc.wrap_t = ToInsigneWrap(desc.wrapT);
				break;
			}
			default:
				break;
			}

			floral::relative_path texturePath = floral::build_relative_path(desc.texturePath);
			insigne::texture_handle_t tex = tex_loader::LoadCBTexture(i_fs, texturePath, texDesc, i_ioAllocator, true);
			insigne::dispatch_render_pass();
			insigne::helpers::assign_texture(o_mat->material, desc.identifier, tex);
		}
	}

	return true;
}

template <class TFileSystem, class TMemoryAllocator>
const bool CreateMaterial(MaterialShaderPair* o_mat, TFileSystem* i_fs, const mat_parser::MaterialDescription& i_matDesc, TMemoryAllocator* i_dataAllocator)
{
	return CreateMaterial(o_mat, i_fs, i_matDesc, (TMemoryAllocator*)nullptr, i_dataAllocator);
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
