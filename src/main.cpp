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
}

void run(event_buffer_t* i_evtBuffer)
{
	using namespace stone;
	using namespace calyx;

	interact_event_t event;

	static bool appRunning = false;

	while (true) // TODO: exiting event
	{
		// event processing
		while (i_evtBuffer->try_pop_into(event)) {
			switch (event.event_type) {
				case interact_event_e::window_lifecycle:
				{
					life_cycle_event_type_e lifeCycleEvent = (life_cycle_event_type_e)event.payload;
					switch (lifeCycleEvent) {
						case life_cycle_event_type_e::pause:
						{
							appRunning = false;
							CLOVER_VERBOSE("Logic will be paused");
							break;
						}

						case life_cycle_event_type_e::resume:
						{
							appRunning = true;
							CLOVER_VERBOSE("Logic will be resumed, but Insigne may still block the frame update");
							break;
						}

						case life_cycle_event_type_e::display_update:
						{
							CLOVER_VERBOSE("Insigne will be updated");
							s_Controller->IOEvents.OnDisplayChanged();
							break;
						}

						case life_cycle_event_type_e::focus_gain:
						{
							CLOVER_VERBOSE("Insigne may be resumed");
							s_Controller->IOEvents.OnFocusChanged(true);
							break;
						}

						case life_cycle_event_type_e::focus_lost:
						{
							CLOVER_VERBOSE("Insigne may be stopped");
							s_Controller->IOEvents.OnFocusChanged(false);
							break;
						}

						default:
							break;
					}
					break;
				}

				default:
					break;
			}
		}

		// the app
		if (appRunning)
		{
			s_Controller->IOEvents.OnFrameStep(16.6f);
		}
	}

#if 0
	while (true) {
		// event processing
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
				
				case calyx::interact_event_e::window_lifecycle:
				{
					if (event.payload == 0) { // onPause
						readyToRender = false;
					} else if (event.payload == 1) { // onResume
						readyToRender = true;
					}
					break;
				}
				default:
					break;
			};
		}
		
		if (readyToRender)
		{
			// logic and render commands building
			s_Controller->IOEvents.OnFrameStep(16.6f);
		}
	}
#endif
}

void clean_up()
{
	using namespace stone;
	s_Controller->IOEvents.OnCleanUp(1);
}

}
