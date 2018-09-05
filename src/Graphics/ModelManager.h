#pragma once

#include <floral.h>

#include "IModelManager.h"
#include "IMaterialManager.h"
#include "Memory/MemorySystem.h"

namespace stone {

class ModelManager : public IModelManager {
public:
	ModelManager(IMaterialManager* i_materialManager);
	~ModelManager();

	void									Initialize() override;
	insigne::surface_handle_t				CreateSingleSurface(const_cstr i_surfPath) override;

	Model*									CreateModel(const floral::path& i_path) override;

private:
	LinearArena*							m_MemoryArena;

private:
	IMaterialManager*							m_MaterialManager;
};

}
