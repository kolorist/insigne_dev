#pragma once

#include <floral/stdaliases.h>
#include <floral/io/filesystem.h>

#include <insigne/commons.h>

#include "Memory/MemorySystem.h"

namespace stone
{
// ----------------------------------------------------------------------------

class ImDrawer2D
{
public:
    struct InitializeData
    {
        floral::filesystem<FreelistArena>*      fileSystem;
        LinearArena*                            memoryArena;
    };

public:
    ImDrawer2D();
    ~ImDrawer2D();

    void                                        Initialize(const InitializeData& i_initData);

private:
    struct XFormData {
        floral::mat4x4f							WVP;
    };
    XFormData								    m_XFormData;
    insigne::ub_handle_t						m_UB;
    insigne::vb_handle_t                        m_VB;
    insigne::ib_handle_t                        m_IB;

    insigne::shader_handle_t					m_Shader;
    insigne::material_desc_t					m_Material;

private:
    LinearArena*                                m_MemoryArena;
};

// ----------------------------------------------------------------------------
}
