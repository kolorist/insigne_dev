#include "Component.h"

namespace stone {

	static floral::crc_string s_name = floral::crc_string("Component");

	void Component::Update(f32 i_deltaMs)
	{
		// nothing
	}

	const floral::crc_string& Component::GetName()
	{
		return s_name;
	}

}
