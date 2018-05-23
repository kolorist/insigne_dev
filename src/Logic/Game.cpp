#include "Game.h"

#include <insigne/commons.h>

#include "Graphics/PBRMaterial.h"
#include "GameObject/GameObject.h"
#include "GameObject/VisualComponent.h"

namespace stone {
	Game::Game(IModelManager* i_modelManager, MaterialManager* i_materialManager)
		: m_ModelManager(i_modelManager)
		, m_MaterialManager(i_materialManager)
	{
	}

	Game::~Game()
	{
	}

	void Game::Initialize()
	{
		m_GameObjects = g_SceneResourceAllocator.allocate<GameObjectArray>(8, &g_SceneResourceAllocator);
		m_VisualComponents = g_SceneResourceAllocator.allocate<VisualComponentArray>(8, &g_SceneResourceAllocator);

		GameObject* newGO = g_SceneResourceAllocator.allocate<GameObject>();
		VisualComponent* newVC = g_SceneResourceAllocator.allocate<VisualComponent>();

		insigne::surface_handle_t shdl = m_ModelManager->CreateSingleSurface("gfx/envi/models/demo/cube.cbobj");
		IMaterial* mat = m_MaterialManager->CreateMaterial<PBRMaterial>("shaders/internal/fallback");
		newVC->Initialize(shdl, mat);

		newGO->AddComponent(newVC);
	}

	void Game::Update(f32 i_deltaMs)
	{
		for (u32 i = 0; i < m_GameObjects->get_size(); i++) {
			(*m_GameObjects)[i]->Update(i_deltaMs);
		}
	}

	void Game::Render()
	{
		for (u32 i = 0; i < m_VisualComponents->get_size(); i++) {
		(*m_VisualComponents)[i]->Render();
		}
	}
}
