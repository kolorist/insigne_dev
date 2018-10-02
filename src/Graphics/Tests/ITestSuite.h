#pragma once

#include <floral.h>

namespace stone {

class ITestSuite {
	public:
		virtual void							OnInitialize() = 0;
		virtual void							OnUpdate(const f32 i_deltaMs) = 0;
		virtual void							OnRender(const f32 i_deltaMs) = 0;
		virtual void							OnCleanUp() = 0;
};

}
