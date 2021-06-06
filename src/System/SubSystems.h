#pragma once

#include <floral/stdaliases.h>
#include <floral/io/filesystem.h>

#include "Memory/MemorySystem.h"

namespace font_renderer
{
class FontRenderer;
}

namespace stone
{
// ----------------------------------------------------------------------------

struct SubSystems
{
    floral::filesystem<FreelistArena>*          fileSystem;
    font_renderer::FontRenderer*                fontRenderer;
};

// ----------------------------------------------------------------------------
}
