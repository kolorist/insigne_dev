#include "SurfaceDefinitions.h"

namespace insigne {

	namespace detail {
		template <>
			gpu_command_buffer_t draw_command_buffer_t<stone::ImGuiSurface>::command_buffer[BUFFERED_FRAMES];

		template <>
			gpu_command_buffer_t draw_command_buffer_t<stone::SolidSurface>::command_buffer[BUFFERED_FRAMES];

		template <>
			gpu_command_buffer_t draw_command_buffer_t<stone::SkyboxSurface>::command_buffer[BUFFERED_FRAMES];

		template <>
			gpu_command_buffer_t draw_command_buffer_t<stone::SSSurface>::command_buffer[BUFFERED_FRAMES];

	}

}
