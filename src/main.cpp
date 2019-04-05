#include <calyx/life_cycle.h>
#include <calyx/context.h>
#include <calyx/events.h>

#include <floral.h>
#include <clover.h>
#include <lotus/profiler.h>

#include <thread/mutex.h>
#include <thread/condition_variable.h>
#include <atomic>

#include "Memory/MemorySystem.h"
#include "System/Controller.h"
#include "Application.h"

namespace stone {

static Controller*								s_Controller;
static Application*								s_Application;

}

namespace calyx {

std::atomic_bool								s_logicResumed(false);

bool											s_inputResumed = false;
floral::mutex									s_inputResumedMtx;
floral::condition_variable						s_inputResumedCdv;

bool											s_gpuReady = false;;
bool											s_logicActive = false;

//----------------------------------------------
void FlushLogicThread()
{
	CLOVER_VERBOSE("Flushing LogicThread (update)...");
	while (s_logicResumed.load(std::memory_order_acquire))
	{
	}
	CLOVER_VERBOSE("LogicThread (update) flushed");
}

void TryWakeLogicThread()
{
	floral::lock_guard guard(s_inputResumedMtx);
	if (!s_inputResumed)
	{
		CLOVER_VERBOSE("Waking up the LogicThread");
		s_inputResumed = true;
		s_inputResumedCdv.notify_one();
	}
	else
	{
		CLOVER_VERBOSE("LogicThread already awakes");
	}
}

void UpdateLogicThreadFlags()
{
	using namespace stone;

	if (s_logicActive && s_gpuReady)
	{
		bool expected = false;
		if (s_logicResumed.compare_exchange_strong(expected, true, std::memory_order_relaxed))
		{
			CLOVER_VERBOSE("LogicThread (update) will be resumed");
			s_Controller->IOEvents.OnResume();
		}
	}
	else
	{
		bool expected = true;
		if (s_logicResumed.compare_exchange_strong(expected, false, std::memory_order_release))
		{
			CLOVER_VERBOSE("LogicThread (update) will be paused");
			s_Controller->IOEvents.OnPause();
		}
	}

	if (s_logicActive == false && s_gpuReady == false)
	{
		if (s_inputResumed)
		{
			s_inputResumed = false;
			CLOVER_VERBOSE("LogicThread (input handling) will be paused");
		}
	}
	else
	{
		if (!s_inputResumed)
		{
			s_inputResumed = true;
			CLOVER_VERBOSE("LogicThread (input handling) will be resumed");
		}
	}
}

void CheckAndPauseLogicThread()
{
	floral::lock_guard guard(s_inputResumedMtx);
	while (!s_inputResumed)
	{
		CLOVER_VERBOSE("Entire LogicThread paused");
		s_inputResumedCdv.wait(s_inputResumedMtx);
		CLOVER_VERBOSE("LogicThread (input handling) resumed");
	}
}

void UpdateLogic(event_buffer_t* i_evtBuffer)
{
	event_buffer_t::queue_t& ioBuffer = i_evtBuffer->flip();
	calyx::event_t eve;
	while (ioBuffer.try_pop_into(eve))
	{
		switch(eve.type)
		{
			case calyx::event_type_e::interact:
			{
				break;
			}

			case calyx::event_type_e::lifecycle:
			{
				switch (eve.lifecycle_event_data.inner_type)
				{
					case calyx::lifecycle_event_type_e::pause:
					{
						s_logicActive = false;
						UpdateLogicThreadFlags();
						break;
					}

					case calyx::lifecycle_event_type_e::resume:
					{
						s_logicActive = true;
						UpdateLogicThreadFlags();
						break;
					}

					case calyx::lifecycle_event_type_e::surface_ready:
					{
						s_gpuReady = true;
						UpdateLogicThreadFlags();
						break;
					}

					case calyx::lifecycle_event_type_e::surface_destroyed:
					{
						s_gpuReady = false;
						UpdateLogicThreadFlags();
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
}

//----------------------------------------------

void setup_surface(context_attribs* io_commonCtx)
{
	io_commonCtx->window_title = "demo";
	io_commonCtx->window_width = 1280;
	io_commonCtx->window_height = 720;
	io_commonCtx->window_offset_left = 50;
	io_commonCtx->window_offset_top = 50;
}

void try_wake_mainthread()
{
	TryWakeLogicThread();
}

void flush_mainthread()
{
	FlushLogicThread();
}

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

	while (true) // TODO: exiting event
	{
		CheckAndPauseLogicThread();
		UpdateLogic(i_evtBuffer);
#if 0
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
#endif
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
