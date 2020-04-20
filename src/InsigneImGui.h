#pragma once

#include <floral/stdaliases.h>
#include <floral/gpds/vec.h>
#include <floral/gpds/mat.h>

#include <imgui.h>

namespace stone
{

void											InitializeImGui();
void											UpdateImGui();
void											RenderImGui();

const bool										ImGuiKeyInput(const u32 i_keyCode, const bool i_isDown);
void											ImGuiCharacterInput(const c8 i_charCode);
const bool										ImGuiCursorMove(const u32 i_x, const u32 i_y);
const bool										ImGuiCursorInteract(const bool i_pressed);

bool											DebugVec2f(const char* i_label, floral::vec2f* i_vec, const char* i_fmt = "%2.3f", ImGuiInputTextFlags i_flags = 0);
bool											DebugVec3f(const char* i_label, floral::vec3f* i_vec, const char* i_fmt = "%2.3f", ImGuiInputTextFlags i_flags = 0);
bool											DebugVec4f(const char* i_label, floral::vec4f* i_vec, const char* i_fmt = "%2.3f", ImGuiInputTextFlags i_flags = 0);
bool											DebugMat3fColumnOrder(const char* i_label, floral::mat3x3f* i_mat);
bool											DebugMat4fColumnOrder(const char* i_label, floral::mat4x4f* i_mat);
bool											DebugMat3fRowOrder(const char* i_label, floral::mat3x3f* i_mat);
bool											DebugMat4fRowOrder(const char* i_label, floral::mat4x4f* i_mat);

const bool										DidImGuiConsumeMouse();

}
