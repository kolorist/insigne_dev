#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "IProbesBaker.h"
#include "Memory/MemorySystem.h"
#include "Camera.h"
#include "RenderData.h"

namespace stone {

class Game;
class IModelManager;

class ProbesBaker : public IProbesBaker {
	public:
		ProbesBaker(Game* i_game, IModelManager* i_modelManager);
		~ProbesBaker();

		void									Initialize(const floral::aabb3f& i_sceneAABB) override;
		void									Render() override;
		void									RenderProbes(Camera* i_camera) override;
		void									CalculateSHs() override;

		insigne::framebuffer_handle_t			GetMegaFramebuffer() override;

	private:
		Game*									m_Game;
		IModelManager*							m_ModelManager;
		insigne::framebuffer_handle_t			m_ProbesFramebuffer;
		bool									m_IsReady;

		struct ProbeCamera {
			Camera								posXCam, negXCam;
			Camera								posYCam, negYCam;
			Camera								posZCam, negZCam;
		};

		floral::fixed_array<ProbeCamera, LinearArena>	m_ProbeCameras;

		Model*									m_SHProbe;

	private:
		LinearArena*							m_MemoryArena;
};

}
