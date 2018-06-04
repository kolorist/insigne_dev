#pragma once

#include <floral.h>

namespace stone {
	class PostFXManager : public IPostFXManager {
		public:
			PostFXManager();
			~PostFXManager();

			void								Initialize() override;
	};
}
