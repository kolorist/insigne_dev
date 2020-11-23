#pragma once

#include <floral/stdaliases.h>
#include <floral/gpds/vec.h>
#include <floral/gpds/mat.h>
#include <floral/io/filesystem.h>
#include <floral/containers/fast_array.h>
#include <floral/containers/text.h>

#include <imgui.h>

#include "Memory/MemorySystem.h"

namespace stone
{

void											InitializeImGui(floral::filesystem<FreelistArena>* i_fs);
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
void											PlotValuesWrap(const_cstr i_title, f32* i_values, const f32 i_minValue, const f32 i_maxValue, const size i_arraySize, const s32 i_height,
													const s32 i_startIdx = 0, const ImU32 i_labelColor = 0xFFFFFFFF, const ImU32 i_lineColor = 0xFFFFFFFF);

const bool										DidImGuiConsumeMouse();

// -----------------------------------------------------------------------------

template <class TAllocator>
struct DebugLogWindow
{
	floral::fast_dynamic_text_buffer<TAllocator>	Buffer;
	floral::fast_dynamic_array<ssize, TAllocator>	LineOffsets;
	bool										AutoScroll;

	DebugLogWindow(TAllocator* allocator);
	void										Clear();
	void										AddLog(const_cstr logStr);
	void										Draw(const_cstr title);
};

}

#include "InsigneImGui.hpp"
