#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "IPostFXManager.h"

namespace stone {

class ToneMappingMaterial;
class GammaCorrectionMaterial;
class MaterialManager;

class PostFXManager : public IPostFXManager {
	public:
		PostFXManager(MaterialManager* i_materialManager);
		~PostFXManager();

		void								Initialize() override;
		const insigne::framebuffer_handle_t	GetMainFramebuffer() override	{ return m_MainFramebuffer; }
		void								Render() override;
		void								RenderFinalPass() override;

	private:
		MaterialManager*					m_MaterialManager;

		insigne::framebuffer_handle_t		m_MainFramebuffer;

		ToneMappingMaterial*				m_TonemapMaterial;
		insigne::framebuffer_handle_t		m_TonemapBuffer;

		GammaCorrectionMaterial*			m_GammaCorrectionMaterial;
		insigne::framebuffer_handle_t		m_GammaCorrectionBuffer;
};

}
