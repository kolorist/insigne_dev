#ifndef __MODEL_BAKER_COMMONS_H__
#define __MODEL_BAKER_COMMONS_H__

#include <floral.h>

#include "Memory/MemorySystem.h"

namespace baker
{

struct Vertex {
	floral::vec3f							Position;
	floral::vec3f							Normal;
	floral::vec2f							TexCoord;

	const bool operator==(const Vertex& other) {
		return (Position == other.Position
				&& Normal == other.Normal
				&& TexCoord == other.TexCoord);
	}
};

template <class DType>
using FixedArray = floral::fixed_array<DType, FreelistArena>;

template <class DType>
using DynamicArray = floral::dynamic_array<DType, FreelistArena>;

}

#endif // __MODEL_BAKER_COMMONS_H__
