#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "IPostFXManager.h"

namespace stone {
	class PostFXManager : public IPostFXManager {
		public:
			PostFXManager();
			~PostFXManager();

			void								Initialize() override;
			const insigne::framebuffer_handle_t	GetMainFramebuffer() override	{ return m_MainFramebuffer; }

		private:
			insigne::framebuffer_handle_t		m_MainFramebuffer;
	};
}
