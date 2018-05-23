#include "PBRMaterial.h"

#include <insigne/render.h>

namespace stone {

	void PBRMaterial::Initialize(insigne::shader_handle_t i_shaderHdl)
	{
		m_MaterialHandle = insigne::create_material(i_shaderHdl);
	}

	insigne::shader_param_list_t* PBRMaterial::BuildShaderParamList()
	{
		insigne::shader_param_list_t* paramList = insigne::allocate_shader_param_list(4);
		paramList->push_back(insigne::shader_param_t("iu_PerspectiveWVP", insigne::param_data_type_e::param_mat4));
		paramList->push_back(insigne::shader_param_t("iu_TransformMat", insigne::param_data_type_e::param_mat4));

		return paramList;
	}

}
