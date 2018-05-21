#pragma once

namespace stone {
	class IMaterialManager {
		public:
			virtual IMaterial*					CreateMaterial(const_cstr i_matPath) = 0;
	};
}
