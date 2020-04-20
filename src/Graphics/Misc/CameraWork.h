#pragma once

#include <floral/stdaliases.h>
#include <floral/gpds/vec.h>
#include <floral/gpds/mat.h>
#include <floral/gpds/camera.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace misc
{
// ------------------------------------------------------------------

class CameraWork : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "camera work";

public:
	CameraWork();
	~CameraWork();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	void										_CalculateCamera();
	void										_ResetValues();

private:
	bool										m_UseRightHanded;
	bool										m_UsePerspectiveProjection;
	f32											m_GridSpacing;
	s32											m_GridRange;

	floral::vec3f								m_LookAt;
	floral::vec3f								m_UpDirection;
	floral::vec3f								m_CameraPosition;

	f32											m_Near, m_Far;
	f32											m_FovY, m_AspectRatio;
	f32											m_Left, m_Right, m_Top, m_Bottom;

	floral::mat4x4f								m_ViewMatrix;
	floral::mat4x4f								m_ProjectionMatrix;
	floral::mat4x4f								m_ViewProjectionMatrix;
};

// ------------------------------------------------------------------
}
}
