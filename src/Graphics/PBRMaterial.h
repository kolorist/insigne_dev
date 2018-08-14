#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "IMaterial.h"

namespace stone {
	class PBRMaterial : public IMaterial {
		public:
			void								Initialize(insigne::shader_handle_t i_shaderHdl) override;

			static insigne::shader_param_list_t*	BuildShaderParamList();

			void								SetWVP(const floral::mat4x4f& i_wvp);
			void								SetTransform(const floral::mat4x4f& i_xform);

			void								SetBaseColorTex(const insigne::texture_handle_t& i_tex);
			void								SetMetallicTex(const insigne::texture_handle_t& i_tex);
			void								SetRoughnessTex(const insigne::texture_handle_t& i_tex);
			void								SetLightDirection(const floral::vec3f& i_v);
			void								SetLightIntensity(const floral::vec3f& i_v);
			void								SetCameraPosition(const floral::vec3f& i_v);

			void								SetIrradianceMap(const insigne::texture_handle_t& i_tex);
			void								SetSpecularMap(const insigne::texture_handle_t& i_tex);

		private:
			// vertex shader
			insigne::param_id					m_ParamWVP;
			insigne::param_id					m_ParamXformMatrix;

			// fragment shader
			insigne::param_id					m_TexBaseColor;
			insigne::param_id					m_TexMetallic;
			insigne::param_id					m_TexRoughness;

			insigne::param_id					m_LightDirection;
			insigne::param_id					m_LightIntensity;
			insigne::param_id					m_CameraPosition;

			insigne::param_id					m_IrrMap;
			insigne::param_id					m_SpecMap;
	};
}
