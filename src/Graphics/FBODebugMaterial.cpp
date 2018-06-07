#include "FBODebugMaterial.h"

#include <insigne/render.h>

namespace stone {
	void FBODebugMaterial::Initialize(insigne::shader_handle_t i_shaderHdl)
	{
		m_MaterialHandle = insigne::create_material(i_shaderHdl);
		
		m_ColorTex0 = insigne::get_material_param<insigne::texture_handle_t>(m_MaterialHandle, "iu_ColorTex0");
	}

	insigne::shader_param_list_t* FBODebugMaterial::BuildShaderParamList()
	{
		insigne::shader_param_list_t* paramList = insigne::allocate_shader_param_list(2);

		paramList->push_back(insigne::shader_param_t("iu_ColorTex0", insigne::param_data_type_e::param_sampler2d));
		
		return paramList;
	}

	// -----------------------------------------
	void FBODebugMaterial::SetColorTex0(insigne::texture_handle_t i_tex)
	{
		insigne::set_material_param(m_MaterialHandle, m_ColorTex0, i_tex);
	}
}
