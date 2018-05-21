#include "SurfaceDefinitions.h"

namespace insigne {
	template <>
	gpu_command_buffer_t draw_command_buffer_t<stone::ImGuiSurface>::command_buffer[BUFFERED_FRAMES];

	template <>
	gpu_command_buffer_t draw_command_buffer_t<stone::SolidSurface>::command_buffer[BUFFERED_FRAMES];
}
