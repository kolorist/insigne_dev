#include "Game.h"

namespace stone {
	Game::Game()
	{
	}

	Game::~Game()
	{
	}

	void Game::Update(f32 i_deltaMs)
	{
		for (u32 i = 0; i < m_GameObjects.get_size(); i++) {
			m_GameObjects[i]->Update(i_deltaMs);
		}
	}

	void Game::Render()
	{
		for (u32 i = 0; i < m_VisualComponents.get_size(); i++) {
			m_VisualComponents[i]->Render();
		}
	}
}
