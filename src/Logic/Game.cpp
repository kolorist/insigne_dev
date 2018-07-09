#include "Game.h"

#include <insigne/commons.h>
#include <lotus/profiler.h>

#include "Graphics/PBRMaterial.h"
#include "GameObject/GameObject.h"
#include "GameObject/VisualComponent.h"
#include "GameObject/PlateComponent.h"

namespace stone {
	Game::Game(IModelManager* i_modelManager, MaterialManager* i_materialManager, ITextureManager* i_textureManager)
		: m_ModelManager(i_modelManager)
		, m_MaterialManager(i_materialManager)
		, m_TextureManager(i_textureManager)
	{
	}

	Game::~Game()
	{
	}

	void Game::Initialize()
	{
		m_GameObjects = g_SceneResourceAllocator.allocate<GameObjectArray>(25, &g_SceneResourceAllocator);
		m_VisualComponents = g_SceneResourceAllocator.allocate<VisualComponentArray>(25, &g_SceneResourceAllocator);

		insigne::texture_handle_t albedo = m_TextureManager->CreateTexture("gfx/go/textures/demo/limestone_albedo.cbtex");
		insigne::texture_handle_t metallic = m_TextureManager->CreateTexture("gfx/go/textures/demo/limestone_metalness.cbtex");
		insigne::texture_handle_t roughness = m_TextureManager->CreateTexture("gfx/go/textures/demo/limestone_roughness.cbtex");
		insigne::texture_handle_t ao = m_TextureManager->CreateTexture("gfx/go/textures/demo/limestone_ao.cbtex");
		insigne::texture_handle_t normal = m_TextureManager->CreateTexture("gfx/go/textures/demo/limestone_normal.cbtex");
		for (u32 i = 0; i < 5; i++)
			for (u32 j = 0; j < 5; j++) {
				GameObject* newGO = g_SceneResourceAllocator.allocate<GameObject>();
				VisualComponent* newVC = g_SceneResourceAllocator.allocate<VisualComponent>();
				PlateComponent* newPC = g_SceneResourceAllocator.allocate<PlateComponent>();

				insigne::surface_handle_t shdl = m_ModelManager->CreateSingleSurface("gfx/go/models/demo/stoneplate.cbobj");

				IMaterial* mat = m_MaterialManager->CreateMaterial<PBRMaterial>("shaders/lighting/pbr_cook_torrance");
				PBRMaterial* pbrMat = (PBRMaterial*)mat;
				pbrMat->SetBaseColorTex(albedo);
				pbrMat->SetMetallicTex(metallic);
				pbrMat->SetRoughnessTex(roughness);

				newVC->Initialize(shdl, mat);
				newVC->SetPosition(floral::vec3f(-0.8f + 0.4f * i, -0.3f, -0.8f + 0.4f * j));
				newVC->SetScaling(floral::vec3f(0.175f, 0.175f, 0.175f));
				newPC->Initialize(newVC, 1000.0f + (i * 5 + j) * 200.0f, 500.0f);

				newGO->AddComponent(newVC);
				newGO->AddComponent(newPC);
				m_VisualComponents->push_back(newVC);
				m_GameObjects->push_back(newGO);
			}

		m_SkyboxSurface = m_ModelManager->CreateSingleSurface("gfx/go/models/demo/cube.cbobj");
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
			PROFILE_SCOPE(VisualComponentRender);
			(*m_VisualComponents)[i]->Render();
		}

		// then render the skybox
		// actually, the order of calling rendering of insigne doesn't matter
		
	}
}
