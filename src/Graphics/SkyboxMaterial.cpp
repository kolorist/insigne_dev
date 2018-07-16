#include "SkyboxMaterial.h"

#include <insigne/render.h>

namespace stone {
	void SkyboxMaterial::Initialize(insigne::shader_handle_t i_shaderHdl)
	{
		m_MaterialHandle = insigne::create_material(i_shaderHdl);

		// vertex shader
		m_ParamWVP = insigne::get_material_param<floral::mat4x4f>(m_MaterialHandle, "iu_PerspectiveWVP");
		m_ParamXformMatrix = insigne::get_material_param<floral::mat4x4f>(m_MaterialHandle, "iu_TransformMat");

		// fragment shader
		m_TexBaseColor = insigne::get_material_param<insigne::texture_handle_t>(m_MaterialHandle, "iu_TexBaseColor");
	}

	insigne::shader_param_list_t* SkyboxMaterial::BuildShaderParamList()
	{
		insigne::shader_param_list_t* paramList = insigne::allocate_shader_param_list(4);
		// vertex shader
		paramList->push_back(insigne::shader_param_t("iu_PerspectiveWVP", insigne::param_data_type_e::param_mat4));
		paramList->push_back(insigne::shader_param_t("iu_TransformMat", insigne::param_data_type_e::param_mat4));

		// fragment shader
		paramList->push_back(insigne::shader_param_t("iu_TexBaseColor", insigne::param_data_type_e::param_sampler2d));

		return paramList;
	}

	// -----------------------------------------
	void SkyboxMaterial::SetWVP(const floral::mat4x4f& i_wvp)
	{
		insigne::set_material_param(m_MaterialHandle, m_ParamWVP, i_wvp);
	}

	void SkyboxMaterial::SetTransform(const floral::mat4x4f& i_xform)
	{
		insigne::set_material_param(m_MaterialHandle, m_ParamXformMatrix, i_xform);
	}

	void SkyboxMaterial::SetBaseColorTex(const insigne::texture_handle_t& i_tex)
	{
		insigne::set_material_param(m_MaterialHandle, m_TexBaseColor, i_tex);
	}
}
