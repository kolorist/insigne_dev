#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "IProbesBaker.h"

namespace stone {

class ProbesBaker : public IProbesBaker {
	public:
		ProbesBaker();
		~ProbesBaker();

		void Initialize() override;

		insigne::framebuffer_handle_t					GetMegaFramebuffer() override;
		bool IsReady() override;

	private:
		insigne::framebuffer_handle_t			m_ProbesFramebuffer;
		bool									m_IsReady;
};

}
