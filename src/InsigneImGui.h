#pragma once

#include <floral/stdaliases.h>

namespace stone
{

void											InitializeImGui();
void											UpdateImGui();
void											RenderImGui();

void											ImGuiCharacterInput(const c8 i_charCode);
void											ImGuiCursorMove(const u32 i_x, const u32 i_y);
void											ImGuiCursorInteract(const bool i_pressed);
const bool										DidImGuiConsumeMouse();

}
