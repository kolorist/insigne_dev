#pragma once

#include <floral.h>

namespace texbaker {

void ConvertTextureCubeHStrip(const_cstr i_inputTexPath, const_cstr i_outputTexPath, const s32 i_maxMips);
void ConvertTextureCubeVCross(const_cstr i_inputTexPath, const_cstr i_outputTexPath, const s32 i_maxMips);

}
