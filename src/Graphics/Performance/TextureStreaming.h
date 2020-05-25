#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/Tests/ITestSuite.h"
#include "Graphics/MaterialLoader.h"

#include "Memory/MemorySystem.h"

namespace stone
{
namespace perf
{

class TextureStreaming : public ITestSuite
{
public:
	TextureStreaming();
	~TextureStreaming();

	const_cstr									GetName() const override;

	void										OnInitialize(floral::filesystem<FreelistArena>* i_fs) override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return nullptr; }

private:
	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;
	insigne::texture_handle_t					m_Texture;
	mat_loader::MaterialShaderPair				m_MSPair;

	floral::vec3f*								m_TexData;

private:
	FreelistArena*								m_MemoryArena;
	LinearArena*								m_MaterialDataArena;
	LinearArena*								m_TextureDataArena;
	
private:
	ssize										m_BuffersBeginStateId;
	ssize										m_ShadingBeginStateId;
	ssize										m_TextureBeginStateId;
	ssize										m_RenderBeginStateId;
};

}
}
