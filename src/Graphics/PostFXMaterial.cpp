#include "PostFXMaterial.h"

#include <insigne/render.h>

namespace stone {

// ToneMapping
void ToneMappingMaterial::Initialize(insigne::shader_handle_t i_shaderHdl)
{
	m_MaterialHandle = insigne::create_material(i_shaderHdl);

	m_InputTex = insigne::get_material_param<insigne::texture_handle_t>(m_MaterialHandle, "iu_ColorTex0");
	m_Exposure = insigne::get_material_param<f32>(m_MaterialHandle, "iu_Exposure");
}

insigne::shader_param_list_t* ToneMappingMaterial::BuildShaderParamList()
{
	insigne::shader_param_list_t* paramList = insigne::allocate_shader_param_list(2);

	paramList->push_back(insigne::shader_param_t("iu_ColorTex0", insigne::param_data_type_e::param_sampler2d));
	paramList->push_back(insigne::shader_param_t("iu_Exposure", insigne::param_data_type_e::param_float));

	return paramList;
}

void ToneMappingMaterial::SetInputTex(insigne::texture_handle_t i_tex)
{
	insigne::set_material_param(m_MaterialHandle, m_InputTex, i_tex);
}

void ToneMappingMaterial::SetExposure(const f32 i_val)
{
	insigne::set_material_param(m_MaterialHandle, m_Exposure, i_val);
}

// GammaCorrection
void GammaCorrectionMaterial::Initialize(insigne::shader_handle_t i_shaderHdl)
{
	m_MaterialHandle = insigne::create_material(i_shaderHdl);

	m_InputTex = insigne::get_material_param<insigne::texture_handle_t>(m_MaterialHandle, "iu_ColorTex0");
}

insigne::shader_param_list_t* GammaCorrectionMaterial::BuildShaderParamList()
{
	insigne::shader_param_list_t* paramList = insigne::allocate_shader_param_list(2);

	paramList->push_back(insigne::shader_param_t("iu_ColorTex0", insigne::param_data_type_e::param_sampler2d));

	return paramList;
}

void GammaCorrectionMaterial::SetInputTex(insigne::texture_handle_t i_tex)
{
	insigne::set_material_param(m_MaterialHandle, m_InputTex, i_tex);
}

}
