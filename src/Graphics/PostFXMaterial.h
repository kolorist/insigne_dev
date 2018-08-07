#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "IMaterial.h"

namespace stone {

class ToneMappingMaterial : public IMaterial {
	public:
		void									Initialize(insigne::shader_handle_t i_shaderHdl) override;
		static insigne::shader_param_list_t*	BuildShaderParamList();
		void									SetInputTex(insigne::texture_handle_t i_tex);
		void									SetExposure(const f32 i_val);

	private:
		insigne::param_id						m_InputTex;
		insigne::param_id						m_Exposure;
};

class GammaCorrectionMaterial : public IMaterial {
	public:
		void									Initialize(insigne::shader_handle_t i_shaderHdl) override;
		static insigne::shader_param_list_t*	BuildShaderParamList();
		void									SetInputTex(insigne::texture_handle_t i_tex);

	private:
		insigne::param_id						m_InputTex;
};

}
