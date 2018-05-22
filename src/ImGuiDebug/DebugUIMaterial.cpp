#include "DebugUIMaterial.h"

#include <insigne/render.h>

namespace stone {

	void DebugUIMaterial::Initialize(insigne::shader_handle_t i_shaderHdl) {
		m_MaterialHandle = insigne::create_material(i_shaderHdl);

		m_ParamWVP = insigne::get_material_param<floral::mat4x4f>(m_MaterialHandle, "iu_DebugOrthoWVP");
		m_ParamTex = insigne::get_material_param<insigne::texture_handle_t>(m_MaterialHandle, "iu_Tex");
	}

	insigne::shader_param_list_t* DebugUIMaterial::BuildShaderParamList()
	{
		insigne::shader_param_list_t* paramList = insigne::allocate_shader_param_list(4);
		paramList->push_back(insigne::shader_param_t("iu_Tex", insigne::param_data_type_e::param_sampler2d));
		paramList->push_back(insigne::shader_param_t("iu_DebugOrthoWVP", insigne::param_data_type_e::param_mat4));

		return paramList;
	}

	// -----------------------------------------
	void DebugUIMaterial::SetDebugOrthoWVP(const floral::mat4x4f& i_wvp)
	{
		insigne::set_material_param(m_MaterialHandle, m_ParamWVP, i_wvp);
	}

	void DebugUIMaterial::SetTexture(const insigne::texture_handle_t& i_tex)
	{
		insigne::set_material_param(m_MaterialHandle, m_ParamTex, i_tex);
	}

}
