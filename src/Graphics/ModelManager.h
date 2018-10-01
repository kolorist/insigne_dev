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
	floral::fixed_array<u32, LinearAllocator>			m_DemoIndicesData;
	struct DemoUniformData {
		floral::vec4f							data[4];
	};
	DemoUniformData								m_DemoUniformData;

private:
	IMaterialManager*							m_MaterialManager;
};

}
