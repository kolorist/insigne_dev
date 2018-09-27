#include <life_cycle.h>
#include <context.h>

#include <floral.h>
#include <clover.h>
#include <platform/windows/event_defs.h>
#include <lotus/profiler.h>

#include "Memory/MemorySystem.h"
#include "System/Controller.h"
#include "Application.h"

namespace stone {
	static Controller*							s_Controller;
	static Application*							s_Application;
}

namespace calyx {

	void initialize()
	{
		using namespace stone;
		lotus::init_capture_for_this_thread(0, "main_thread");

		s_Controller = g_PersistanceAllocator.allocate<Controller>();
		s_Application = g_PersistanceAllocator.allocate<Application>(s_Controller);
		s_Controller->IOEvents.OnInitialize(1);
	}

	void run(event_buffer_t* i_evtBuffer)
	{
		using namespace stone;
		calyx::interact_event_t event;
		while (true) {
			while (i_evtBuffer->try_pop_into(event)) {
				switch (event.event_type) {
					case calyx::interact_event_e::cursor_interact:
						{
							if (TEST_BIT(event.payload, CLX_MOUSE_LEFT_BUTTON)) {
								s_Controller->IOEvents.CursorInteract(
										TEST_BIT_BOOL(event.payload, CLX_MOUSE_BUTTON_PRESSED),
										1);
							}
							if (TEST_BIT(event.payload, CLX_MOUSE_RIGHT_BUTTON)) {
								s_Controller->IOEvents.CursorInteract(
										TEST_BIT_BOOL(event.payload, CLX_MOUSE_BUTTON_PRESSED),
										2);
							}
							break;
						}

					case calyx::interact_event_e::cursor_move:
						{
							u32 x = event.payload & 0xFFFF;
							u32 y = (event.payload & 0xFFFF0000) >> 16;
							s_Controller->IOEvents.CursorMove(x, y);
							break;
						}

					case calyx::interact_event_e::scroll:
						{
							break;
						}

					case calyx::interact_event_e::character_input:
						{
							break;
						}

					case calyx::interact_event_e::key_input:
						{
							if (TEST_BIT(event.payload, CLX_KEY)) {
								u32 keyCode = event.payload >> 4;
								if (TEST_BIT(event.payload, CLX_KEY_PRESSED)) {
									s_Controller->IOEvents.KeyInput(keyCode, 0);
								} else if (TEST_BIT(event.payload, CLX_KEY_HELD)) {
									s_Controller->IOEvents.KeyInput(keyCode, 1);
								} else {
									s_Controller->IOEvents.KeyInput(keyCode, 2);
								}
							}
							break;
						}

					default:
						break;
				};
			}

			s_Controller->IOEvents.OnFrameStep(16.6f);
		}
	}

	void clean_up()
	{
		using namespace stone;
		s_Controller->IOEvents.OnCleanUp(1);
	}

}
