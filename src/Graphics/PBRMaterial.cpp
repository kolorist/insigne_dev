#include "PBRMaterial.h"

#include <insigne/render.h>

namespace stone {

	void PBRMaterial::Initialize(insigne::shader_handle_t i_shaderHdl)
	{
		m_MaterialHandle = insigne::create_material(i_shaderHdl);

		m_ParamWVP = insigne::get_material_param<floral::mat4x4f>(m_MaterialHandle, "iu_PerspectiveWVP");
		m_ParamXformMatrix = insigne::get_material_param<floral::mat4x4f>(m_MaterialHandle, "iu_TransformMat");
		m_TestTex = insigne::get_material_param<insigne::texture_handle_t>(m_MaterialHandle, "iu_TestTex");
	}

	insigne::shader_param_list_t* PBRMaterial::BuildShaderParamList()
	{
		insigne::shader_param_list_t* paramList = insigne::allocate_shader_param_list(4);
		paramList->push_back(insigne::shader_param_t("iu_PerspectiveWVP", insigne::param_data_type_e::param_mat4));
		paramList->push_back(insigne::shader_param_t("iu_TransformMat", insigne::param_data_type_e::param_mat4));
		paramList->push_back(insigne::shader_param_t("iu_TestTex", insigne::param_data_type_e::param_sampler2d));

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

	void PBRMaterial::SetTestTex(const insigne::texture_handle_t& i_tex)
	{
		insigne::set_material_param(m_MaterialHandle, m_TestTex, i_tex);
	}

}
