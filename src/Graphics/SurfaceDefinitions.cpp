#include "SurfaceDefinitions.h"

namespace insigne {

namespace detail {

template <>
gpu_command_buffer_t draw_command_buffer_t<stone::DemoSurface>::command_buffer[BUFFERS_COUNT];

}

}
