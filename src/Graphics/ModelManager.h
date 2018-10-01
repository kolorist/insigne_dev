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
	insigne::vb_handle_t						CreateDemoVB() override;

	Model*									CreateModel(const floral::path& i_path, floral::aabb3f& o_aabb) override;

private:
	LinearArena*							m_MemoryArena;

	floral::fixed_array<floral::vec3f, LinearAllocator>	m_DemoData;

private:
	IMaterialManager*							m_MaterialManager;
};

}
