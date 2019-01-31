#pragma once

#include <floral.h>

#include "Memory/MemorySystem.h"

namespace baker
{

typedef floral::dynamic_array<floral::vec3f, FreelistArena>	Vec3Array;
typedef floral::dynamic_array<floral::vec2f, FreelistArena> Vec2Array;
typedef floral::dynamic_array<f32, FreelistArena> F32Array;
typedef floral::dynamic_array<s32, FreelistArena> S32Array;
typedef floral::dynamic_array<const_cstr, FreelistArena> StringArray;

namespace pbrt
{

struct SceneCreationCallbacks
{
	floral::simple_callback<
		void,
		const S32Array& /* indices */,
		const Vec3Array& /* pos */,
		const Vec3Array& /* normal */,
		const Vec2Array& /* uv &*/>				OnNewMesh;
	floral::simple_callback<
		void,
		const_cstr /* ply file name */>			OnNewPlyMesh;
};

}

}

