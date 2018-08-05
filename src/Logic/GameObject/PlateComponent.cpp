#include "PlateComponent.h"

#include <lotus/profiler.h>

namespace stone {

	PlateComponent::PlateComponent()
		: m_VisualComponent(nullptr)
		, m_MovementState(MovementState::Ready)
		, m_ReadyDurMs(0.0f)
		, m_ReadyTimeMs(0.0f)
		, m_LeadInDurMs(0.0f)
		, m_LeadInTimeMs(0.0f)
	{
	}

	PlateComponent::~PlateComponent()
	{
	}

	void PlateComponent::Update(Camera* i_camera, f32 i_deltaMs)
	{
		PROFILE_SCOPE(UpdatePlateComponent);
		switch (m_MovementState) {
			case MovementState::Ready:
				{
					m_ReadyTimeMs += i_deltaMs;
					if (m_ReadyTimeMs >= m_ReadyDurMs) {
						m_ReadyTimeMs = m_ReadyDurMs;
						m_MovementState = MovementState::LeadIn;
					}
					break;
				}

			case MovementState::LeadIn:
				{
					m_LeadInTimeMs += i_deltaMs;
					if (m_LeadInTimeMs >= m_LeadInDurMs) {
						m_LeadInTimeMs = m_LeadInDurMs;
						m_MovementState = MovementState::Hold;
					}
					f32 t = m_LeadInTimeMs / m_LeadInDurMs;
					floral::vec3f cp = m_VisualComponent->GetPosition();
					cp.y = floral::ease_out_sin(t, -0.3f, 0.0f);
					m_VisualComponent->SetPosition(cp);
					break;
				}

			default:
				break;
		}
	}

	void PlateComponent::Initialize(VisualComponent* i_visualComponent, const f32 i_readyDur, const f32 i_leadInDur)
	{
		m_VisualComponent = i_visualComponent;
		m_ReadyDurMs = i_readyDur;
		m_LeadInDurMs = i_leadInDur;
	}

}
