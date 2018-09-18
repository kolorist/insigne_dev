#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "IProbesBaker.h"
#include "Memory/MemorySystem.h"
#include "Camera.h"

namespace stone {

class Game;

class ProbesBaker : public IProbesBaker {
	public:
		ProbesBaker(Game* i_game);
		~ProbesBaker();

		void									Initialize(const floral::aabb3f& i_sceneAABB) override;
		void									Render() override;
		void									CalculateSHs() override;

		insigne::framebuffer_handle_t			GetMegaFramebuffer() override;

	private:
		Game*									m_Game;
		insigne::framebuffer_handle_t			m_ProbesFramebuffer;
		bool									m_IsReady;

		struct ProbeCamera {
			Camera								posXCam, negXCam;
			Camera								posYCam, negYCam;
			Camera								posZCam, negZCam;
		};

		floral::fixed_array<ProbeCamera, LinearArena>	m_ProbeCameras;

	private:
		LinearArena*							m_MemoryArena;
};

}
