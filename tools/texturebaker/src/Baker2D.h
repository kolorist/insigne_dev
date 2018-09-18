#pragma once

#include <floral.h>

namespace texbaker {

void ConvertTexture2D(const_cstr i_inputTexPath, const_cstr i_outputTexPath, const s32 i_maxMips);
void ComputeSH(const_cstr i_inputTexPath);

}
