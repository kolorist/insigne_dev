#include "VisualComponent.h"

#include <insigne/render.h>
#include <lotus/profiler.h>

#include "Graphics/Camera.h"
#include "Graphics/SurfaceDefinitions.h"

#include "Graphics/RenderData.h"

namespace stone {
	VisualComponent::VisualComponent()
		: m_PositionWS(0.0f, 0.0f, 0.0f)
		, m_RotationWS(0.0f, 0.0f, 0.0f)
		, m_ScalingWS(1.0f, 1.0f, 1.0f)
		, m_Model(nullptr)
	{
	}

	VisualComponent::~VisualComponent()
	{
	}

	void VisualComponent::Update(Camera* i_camera, f32 i_deltaMs)
	{
		PROFILE_SCOPE(UpdateVisualComponent);
		// transform update
		m_Transform = floral::construct_translation3d(m_PositionWS) * floral::construct_scaling3d(m_ScalingWS);
	}

	void VisualComponent::Render(Camera* i_camera)
	{
		for (u32 i = 0; i < m_Model->surfacesList.get_size(); i++) {
			insigne::material_handle_t surfaceMat = m_Model->surfacesList[i].materialHdl;
			insigne::surface_handle_t surface = m_Model->surfacesList[i].surfaceHdl;
			insigne::param_id wvp = insigne::get_material_param<floral::mat4x4f>(surfaceMat, "iu_PerspectiveWVP");
			insigne::param_id xform = insigne::get_material_param<floral::mat4x4f>(surfaceMat, "iu_TransformMat");

			insigne::set_material_param(surfaceMat, wvp, i_camera->WVPMatrix);
			insigne::set_material_param(surfaceMat, xform, m_Transform);
			insigne::draw_surface<SolidSurface>(surface, surfaceMat);
		}
	}

	void VisualComponent::RenderWithMaterial(Camera* i_camera, insigne::material_handle_t i_ovrMaterial)
	{
		for (u32 i = 0; i < m_Model->surfacesList.get_size(); i++) {
			insigne::material_handle_t surfaceMat = i_ovrMaterial;
			insigne::surface_handle_t surface = m_Model->surfacesList[i].surfaceHdl;
			insigne::param_id wvp = insigne::get_material_param<floral::mat4x4f>(surfaceMat, "iu_PerspectiveWVP");
			insigne::param_id xform = insigne::get_material_param<floral::mat4x4f>(surfaceMat, "iu_TransformMat");

			insigne::set_material_param(surfaceMat, wvp, i_camera->WVPMatrix);
			insigne::set_material_param(surfaceMat, xform, m_Transform);
			insigne::draw_surface<SolidSurface>(surface, surfaceMat);
		}
	}

	void VisualComponent::Initialize(Model* i_model)
	{
		m_Model = i_model;
	}
}
