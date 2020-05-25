#pragma once

#include <floral/stdaliases.h>
#include <floral/io/filesystem.h>

#include "Memory/MemorySystem.h"

namespace stone
{
// -------------------------------------------------------------------

class ICameraMotion;

class ITestSuite
{
public:
	virtual ~ITestSuite() = default;

	virtual const_cstr							GetName() const { return nullptr; }

	virtual void								OnInitialize(floral::filesystem<FreelistArena>* i_fs) = 0;
	virtual void								OnUpdate(const f32 i_deltaMs) = 0;
	virtual void								OnRender(const f32 i_deltaMs) = 0;
	virtual void								OnCleanUp() = 0;

	virtual ICameraMotion*						GetCameraMotion() = 0;
};

// -------------------------------------------------------------------
}
