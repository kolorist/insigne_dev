#pragma once

#include <floral/stdaliases.h>

#include <imgui.h>

namespace profiler
{
// -------------------------------------------------------------------

struct ValuesSet
{
	f32											minValue;
	f32											maxValue;
	f32*										values;
	size										capacity;
	size										writePosition;
};

template <class TAllocator>
ValuesSet* CreateValuesSet(const size i_capacity, TAllocator* i_allocator);

void PushAndPlotValuesSet(ValuesSet* io_valuesSet, const s32 i_height, const ImU32 i_labelColor = 0xFFFFFFFF, const ImU32 i_lineColor = 0xFFFFFFFF);

// -------------------------------------------------------------------
}
