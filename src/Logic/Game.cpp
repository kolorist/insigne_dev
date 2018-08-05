#include "Game.h"

#include <clover.h>
#include <insigne/commons.h>
#include <lotus/profiler.h>

#include "Graphics/PBRMaterial.h"
#include "Graphics/SkyboxMaterial.h"
#include "GameObject/GameObject.h"
#include "GameObject/VisualComponent.h"
#include "GameObject/PlateComponent.h"
#include "GameObject/CameraComponent.h"
#include "GameObject/SkyboxComponent.h"
#include "ImGuiDebug/Debugger.h"

namespace stone {
	Game::Game(IModelManager* i_modelManager, MaterialManager* i_materialManager, ITextureManager* i_textureManager,
			Debugger* i_debugger)
		: m_ModelManager(i_modelManager)
		, m_MaterialManager(i_materialManager)
		, m_TextureManager(i_textureManager)
		, m_Debugger(i_debugger)
		, m_GameObjects(nullptr)
		, m_VisualComponents(nullptr)
	{
	}

	Game::~Game()
	{
	}

	void Game::Initialize()
	{
		m_Debugger->OnRequestLoadDefaultTextures.bind<Game, &Game::RequestLoadDefaultTextures>(this);
		m_Debugger->OnRequestLoadPlateMaterial.bind<Game, &Game::RequestLoadPlateMaterial>(this);
		m_Debugger->OnRequestConstructCamera.bind<Game, &Game::RequestContructCamera>(this);
		m_Debugger->OnRequestLoadModels.bind<Game, &Game::RequestLoadModels>(this);
		m_Debugger->OnRequestLoadAndApplyTextures.bind<Game, &Game::RequestLoadAndApplyTextures>(this);
		m_Debugger->OnRequestLoadSkybox.bind<Game, &Game::RequestLoadSkybox>(this);
		m_Debugger->OnRequestLoadShadingProbes.bind<Game, &Game::RequestLoadShadingProbes>(this);

		m_SkyboxSurface = m_ModelManager->CreateSingleSurface("gfx/go/models/demo/cube.cbobj");
	}

	void Game::Update(f32 i_deltaMs)
	{
		if (m_CameraComponent) {
			m_CameraComponent->Update(nullptr, i_deltaMs);
		}

		if (m_GameObjects) {
			for (u32 i = 0; i < m_GameObjects->get_size(); i++) {
				(*m_GameObjects)[i]->Update(m_CameraComponent->GetCamera(), i_deltaMs);
			}
		}
	}

	void Game::Render()
	{
		if (m_VisualComponents) {
			for (u32 i = 0; i < m_VisualComponents->get_size(); i++) {
				PROFILE_SCOPE(VisualComponentRender);
				(*m_VisualComponents)[i]->Render(m_CameraComponent->GetCamera());
			}
		}

		// then render the skybox
		// actually, the order of calling rendering of insigne doesn't matter
		if (m_SkyboxComponents) {
			for (u32 i = 0; i < m_SkyboxComponents->get_size(); i++) {
				PROFILE_SCOPE(SkyboxComponentRender);
				(*m_SkyboxComponents)[i]->Render(m_CameraComponent->GetCamera());
			}
		}
	}

	void Game::RequestLoadDefaultTextures()
	{
		CLOVER_INFO("Request load default textures...");
		m_DefaultAlbedo = m_TextureManager->CreateTexture("gfx/go/textures/demo/test_albedo.cbtex");
		m_DefaultMetallic = m_TextureManager->CreateTexture("gfx/go/textures/demo/test_attrib.cbtex");
		m_DefaultRoughness = m_TextureManager->CreateTexture("gfx/go/textures/demo/test_attrib.cbtex");
	}

	void Game::RequestLoadPlateMaterial()
	{
		CLOVER_INFO("Request load Plate material...");
		m_PlateMaterial = m_MaterialManager->CreateMaterial<PBRMaterial>("shaders/lighting/pbr_cook_torrance");
		PBRMaterial* pbrMat = (PBRMaterial*)m_PlateMaterial;
		pbrMat->SetBaseColorTex(m_DefaultAlbedo);
		pbrMat->SetMetallicTex(m_DefaultMetallic);
		pbrMat->SetRoughnessTex(m_DefaultRoughness);
	}

	void Game::RequestContructCamera()
	{
		CLOVER_INFO("Request construct a camera...");
		m_CameraComponent = g_SceneResourceAllocator.allocate<CameraComponent>();
		m_CameraComponent->Initialize(0.01f, 100.0f, 45.0f, 16.0f / 9.0f);
		m_CameraComponent->SetPosition(floral::vec3f(3.0f, 1.0f, 3.0f));
		m_CameraComponent->SetLookAtDir(floral::vec3f(-3.0f, -1.0f, -3.0f));
		m_Debugger->SetCamera(m_CameraComponent->GetCamera());
	}

	void Game::RequestLoadModels()
	{
		CLOVER_INFO("Request load models...");
		m_GameObjects = g_SceneResourceAllocator.allocate<GameObjectArray>(32, &g_SceneResourceAllocator);
		m_VisualComponents = g_SceneResourceAllocator.allocate<VisualComponentArray>(32, &g_SceneResourceAllocator);
		m_SkyboxComponents = g_SceneResourceAllocator.allocate<SkyboxComponentArray>(4, &g_SceneResourceAllocator);

		m_PlateModel = m_ModelManager->CreateSingleSurface("gfx/go/models/demo/stoneplate.cbobj");

		for (u32 i = 0; i < 5; i++)
			for (u32 j = 0; j < 5; j++) {
				GameObject* newGO = g_SceneResourceAllocator.allocate<GameObject>();
				VisualComponent* newVC = g_SceneResourceAllocator.allocate<VisualComponent>();
				PlateComponent* newPC = g_SceneResourceAllocator.allocate<PlateComponent>();

				newVC->Initialize(m_PlateModel, m_PlateMaterial);
				newVC->SetPosition(floral::vec3f(-0.8f + 0.4f * i, -0.3f, -0.8f + 0.4f * j));
				newVC->SetScaling(floral::vec3f(0.175f, 0.175f, 0.175f));
				newPC->Initialize(newVC, 1000.0f + (i * 5 + j) * 200.0f, 500.0f);

				newGO->AddComponent(newVC);
				newGO->AddComponent(newPC);
				m_VisualComponents->push_back(newVC);
				m_GameObjects->push_back(newGO);
			}

	}

	void Game::RequestLoadAndApplyTextures()
	{
		CLOVER_INFO("Request Load and Apply Texture...");
		m_Albedo = m_TextureManager->CreateTexture("gfx/go/textures/demo/limestone_albedo.cbtex");
		m_Metallic = m_TextureManager->CreateTexture("gfx/go/textures/demo/limestone_metalness.cbtex");
		m_Rougness = m_TextureManager->CreateTexture("gfx/go/textures/demo/limestone_roughness.cbtex");
		m_AO = m_TextureManager->CreateTexture("gfx/go/textures/demo/limestone_ao.cbtex");
		m_Normal = m_TextureManager->CreateTexture("gfx/go/textures/demo/limestone_normal.cbtex");

		PBRMaterial* pbrMat = (PBRMaterial*)m_PlateMaterial;
		pbrMat->SetBaseColorTex(m_Albedo);
		pbrMat->SetMetallicTex(m_Metallic);
		pbrMat->SetRoughnessTex(m_Rougness);
	}

	void Game::RequestLoadSkybox()
	{
		CLOVER_INFO("Request Load Skybox...");
		CLOVER_INFO("Loading Skybox material...");
		m_SkyboxMaterial = m_MaterialManager->CreateMaterial<SkyboxMaterial>("shaders/lighting/skybox");
		m_SkyboxAlbedo = m_TextureManager->CreateTextureCube("gfx/envi/textures/demo/ditch_river.cbskb");

		SkyboxMaterial* skbMat = (SkyboxMaterial*)m_SkyboxMaterial;
		skbMat->SetBaseColorTex(m_SkyboxAlbedo);

		CLOVER_INFO("Loading Skybox surface...");
		m_SkyboxSurface = m_ModelManager->CreateSingleSurface("gfx/envi/models/demo/cube.cbobj");

		// skybox gameobject
		{
			GameObject* newGO = g_SceneResourceAllocator.allocate<GameObject>();
			SkyboxComponent* newSC = g_SceneResourceAllocator.allocate<SkyboxComponent>();

			newSC->Initialize(m_SkyboxSurface, m_SkyboxMaterial);

			newGO->AddComponent(newSC);
			m_SkyboxComponents->push_back(newSC);
			m_GameObjects->push_back(newGO);
		}
	}

	void Game::RequestLoadShadingProbes()
	{
		CLOVER_INFO("Request Load Shading Probes...");
		insigne::texture_handle_t texHdl = m_TextureManager->CreateMipmapedProbe("gfx/envi/textures/demo/irrmap_grace.cbprb");
		insigne::texture_handle_t texHdl2 = m_TextureManager->CreateMipmapedProbe("gfx/envi/textures/demo/specmap_grace.cbprb");
	}

}
