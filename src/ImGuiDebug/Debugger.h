#pragma once

#include <floral.h>
#include <imgui.h>
#include <insigne/commons.h>
#include <lotus/events.h>

#include "Memory/MemorySystem.h"


namespace stone {

	struct Camera;
	class MaterialManager;
	class ITextureManager;

	extern floral::fixed_array<lotus::unpacked_event, LinearAllocator>	s_profileEvents[4];

	class Debugger {
		public:
			Debugger(MaterialManager* i_materialManager, ITextureManager* i_textureManager);
			~Debugger();

			void								Initialize();
			void								Update(f32 i_deltaMs);
			void								Render(f32 i_deltaMs);

			void								OnCharacterInput(c8 i_character);
			void								OnCursorMove(u32 i_x, u32 i_y);
			void								OnCursorInteract(bool i_pressed, u32 i_buttonId);

			void								SetCamera(Camera* i_camera) { m_Camera = i_camera; }

		private:
			static void							RenderImGuiDrawLists(ImDrawData* i_drawData);

		private:
			f32									m_MouseX, m_MouseY;
			bool								m_MousePressed[2];
			bool								m_MouseHeldThisFrame[2];

			Camera*								m_Camera;

			MaterialManager*					m_MaterialManager;
			ITextureManager*					m_TextureManager;

		public:
			floral::simple_callback<void>		OnRequestLoadDefaultTextures;
			floral::simple_callback<void>		OnRequestLoadPlateMaterial;
			floral::simple_callback<void>		OnRequestConstructCamera;
			floral::simple_callback<void>		OnRequestLoadModels;
			floral::simple_callback<void>		OnRequestLoadAndApplyTextures;
			floral::simple_callback<void>		OnRequestLoadSkybox;
			floral::simple_callback<void>		OnRequestLoadShadingProbes;
			floral::simple_callback<void>		OnRequestLoadLUTTexture;
	};

}
