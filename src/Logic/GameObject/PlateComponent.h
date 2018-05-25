#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "Component.h"
#include "VisualComponent.h"

namespace stone {
	class PlateComponent : public Component {
		public:
			enum class MovementState {
				Ready = 0,
				LeadIn,
				Hold,
				LeadOut
			};

		public:
			PlateComponent();
			~PlateComponent();

			void								Update(f32 i_deltaMs);

			void								Initialize(VisualComponent* i_visualComponent, const f32 i_readyDur, const f32 i_leadInDur);

		private:
			VisualComponent*					m_VisualComponent;
			MovementState						m_MovementState;
			f32									m_ReadyDurMs;
			f32									m_ReadyTimeMs;
			f32									m_LeadInDurMs;
			f32									m_LeadInTimeMs;
	};
}
