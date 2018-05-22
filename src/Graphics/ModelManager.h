#pragma once

#include <floral.h>

#include "IModelManager.h"
#include "Memory/MemorySystem.h"

namespace stone {
	class ModelManager : public IModelManager {
		public:
			ModelManager();
			~ModelManager();

			void								Initialize() override;
			insigne::surface_handle_t			CreateSingleSurface(const_cstr i_surfPath) override;

		private:
			LinearArena*						m_MemoryArena;
	};
}
