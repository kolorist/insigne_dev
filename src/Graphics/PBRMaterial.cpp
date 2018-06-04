#include "PBRMaterial.h"

#include <insigne/render.h>

namespace stone {

	void PBRMaterial::Initialize(insigne::shader_handle_t i_shaderHdl)
	{
		m_MaterialHandle = insigne::create_material(i_shaderHdl);

		// vertex shader
		m_ParamWVP = insigne::get_material_param<floral::mat4x4f>(m_MaterialHandle, "iu_PerspectiveWVP");
		m_ParamXformMatrix = insigne::get_material_param<floral::mat4x4f>(m_MaterialHandle, "iu_TransformMat");

		// fragment shader
		m_TexBaseColor = insigne::get_material_param<insigne::texture_handle_t>(m_MaterialHandle, "iu_TexBaseColor");
		m_TexMetallic = insigne::get_material_param<insigne::texture_handle_t>(m_MaterialHandle, "iu_TexMetallic");
		m_TexRoughness = insigne::get_material_param<insigne::texture_handle_t>(m_MaterialHandle, "iu_TexRoughness");
		m_LightDirection = insigne::get_material_param<floral::vec3f>(m_MaterialHandle, "iu_LightDirection");
		m_LightIntensity = insigne::get_material_param<floral::vec3f>(m_MaterialHandle, "iu_LightIntensity");
	}

	insigne::shader_param_list_t* PBRMaterial::BuildShaderParamList()
	{
		insigne::shader_param_list_t* paramList = insigne::allocate_shader_param_list(8);
		// vertex shader
		paramList->push_back(insigne::shader_param_t("iu_PerspectiveWVP", insigne::param_data_type_e::param_mat4));
		paramList->push_back(insigne::shader_param_t("iu_TransformMat", insigne::param_data_type_e::param_mat4));

		// fragment shader
		paramList->push_back(insigne::shader_param_t("iu_TexBaseColor", insigne::param_data_type_e::param_sampler2d));
		paramList->push_back(insigne::shader_param_t("iu_TexMetallic", insigne::param_data_type_e::param_sampler2d));
		paramList->push_back(insigne::shader_param_t("iu_TexRoughness", insigne::param_data_type_e::param_sampler2d));
		paramList->push_back(insigne::shader_param_t("iu_LightDirection", insigne::param_data_type_e::param_vec3));
		paramList->push_back(insigne::shader_param_t("iu_LightIntensity", insigne::param_data_type_e::param_vec3));

		return paramList;
	}

	// -----------------------------------------
	void PBRMaterial::SetWVP(const floral::mat4x4f& i_wvp)
	{
		insigne::set_material_param(m_MaterialHandle, m_ParamWVP, i_wvp);
	}

	void PBRMaterial::SetTransform(const floral::mat4x4f& i_xform)
	{
		insigne::set_material_param(m_MaterialHandle, m_ParamXformMatrix, i_xform);
	}

	void PBRMaterial::SetBaseColorTex(const insigne::texture_handle_t& i_tex)
	{
		insigne::set_material_param(m_MaterialHandle, m_TexBaseColor, i_tex);
	}

	void PBRMaterial::SetMetallicTex(const insigne::texture_handle_t& i_tex)
	{
		insigne::set_material_param(m_MaterialHandle, m_TexMetallic, i_tex);
	}

	void PBRMaterial::SetRoughnessTex(const insigne::texture_handle_t& i_tex)
	{
		insigne::set_material_param(m_MaterialHandle, m_TexRoughness, i_tex);
	}

	void PBRMaterial::SetLightDirection(const floral::vec3f& i_v)
	{
		insigne::set_material_param(m_MaterialHandle, m_LightDirection, i_v);
	}

	void PBRMaterial::SetLightIntensity(const floral::vec3f& i_v)
	{
		insigne::set_material_param(m_MaterialHandle, m_LightIntensity, i_v);
	}

}
