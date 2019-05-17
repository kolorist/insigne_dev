#include <calyx/life_cycle.h>
#include <calyx/context.h>
#include <calyx/events.h>
#include <calyx/platform/windows/event_defs.h>

#include <floral.h>
#include <clover.h>
#include <lotus/profiler.h>

#include <insigne/system.h>

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
bool											s_rendererInited = false;
bool											s_gameInited = false;

//----------------------------------------------
void FlushLogicThread()
{
	LOG_TOPIC("game");
	CLOVER_VERBOSE("Flushing LogicThread (update)...");
	while (s_logicResumed.load(std::memory_order_acquire))
	{
	}
	insigne::wait_finish_dispatching();
	CLOVER_VERBOSE("LogicThread (update) flushed");
}

void TryWakeLogicThread()
{
	floral::lock_guard guard(s_inputResumedMtx);
	LOG_TOPIC("game");
	CLOVER_VERBOSE("Waking up the LogicThread");
	if (!s_inputResumed)
	{
		s_inputResumed = true;
		s_inputResumedCdv.notify_one();
		CLOVER_VERBOSE("WakeUp signal triggered for LogicThread");
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
		if (!s_gameInited)
		{
			s_Controller->IOEvents.OnInitializeGame();
			s_gameInited = true;
		}
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
	using namespace stone;
	LOG_TOPIC("game");

	event_buffer_t::queue_t& ioBuffer = i_evtBuffer->flip();
	calyx::event_t eve;
	while (ioBuffer.try_pop_into(eve))
	{
		LOG_TOPIC("io_event_loop");
		switch(eve.type)
		{
			case calyx::event_type_e::interact:
			{
				switch (eve.interact_event_data.inner_type)
				{
					case calyx::interact_event_e::key_input:
					{
						if (TEST_BIT(eve.interact_event_data.payload, CLX_KEY)) {
							u32 keyCode = eve.interact_event_data.payload >> 4;
							if (TEST_BIT(eve.interact_event_data.payload, CLX_KEY_PRESSED)) {
								s_Controller->IOEvents.KeyInput(keyCode, 0);
							} else if (TEST_BIT(eve.interact_event_data.payload, CLX_KEY_HELD)) {
								s_Controller->IOEvents.KeyInput(keyCode, 1);
							} else {
								s_Controller->IOEvents.KeyInput(keyCode, 2);
							}
						}
						break;
					}

					case calyx::interact_event_e::cursor_interact:
					{
						if (TEST_BIT(eve.interact_event_data.payload, CLX_MOUSE_LEFT_BUTTON)) {
							s_Controller->IOEvents.CursorInteract(
									TEST_BIT_BOOL(eve.interact_event_data.payload, CLX_MOUSE_BUTTON_PRESSED),
									1);
						}
						if (TEST_BIT(eve.interact_event_data.payload, CLX_MOUSE_RIGHT_BUTTON)) {
							s_Controller->IOEvents.CursorInteract(
									TEST_BIT_BOOL(eve.interact_event_data.payload, CLX_MOUSE_BUTTON_PRESSED),
									2);
						}
						break;
					}

					case calyx::interact_event_e::cursor_move:
					{
						u32 x = eve.interact_event_data.payload & 0xFFFF;
						u32 y = (eve.interact_event_data.payload & 0xFFFF0000) >> 16;
						s_Controller->IOEvents.CursorMove(x, y);
						break;
					}
				}
				break;
			}

			case calyx::event_type_e::lifecycle:
			{
				switch (eve.lifecycle_event_data.inner_type)
				{
					case calyx::lifecycle_event_type_e::pause:
					{
						CLOVER_VERBOSE("<pause> event received");
						s_logicActive = false;
						UpdateLogicThreadFlags();
						break;
					}

					case calyx::lifecycle_event_type_e::resume:
					{
						CLOVER_VERBOSE("<resume> event received");
						s_logicActive = true;
						UpdateLogicThreadFlags();
						break;
					}

					case calyx::lifecycle_event_type_e::surface_ready:
					{
						CLOVER_VERBOSE("<surface_ready> event received");
						s_gpuReady = true;
						if (!s_rendererInited)
						{
							s_Controller->IOEvents.OnInitializeRenderer();
							s_rendererInited = true;
						}
						UpdateLogicThreadFlags();
						break;
					}

					case calyx::lifecycle_event_type_e::surface_destroyed:
					{
						CLOVER_VERBOSE("<surface_destroyed> event received");
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

	// frame update
	if (s_logicResumed.load(std::memory_order_relaxed))
	{
		s_Controller->IOEvents.OnFrameStep(16.6f);
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

	LOG_TOPIC("game");
	CLOVER_VERBOSE("Initialize Game's system...");

	s_Controller = g_PersistanceAllocator.allocate<Controller>();
	s_Application = g_PersistanceAllocator.allocate<Application>(s_Controller);

	s_Controller->IOEvents.OnInitializePlatform();
}

void run(event_buffer_t* i_evtBuffer)
{
	using namespace stone;
	using namespace calyx;

	while (true) // TODO: exiting event
	{
		CheckAndPauseLogicThread();
		UpdateLogic(i_evtBuffer);
	}
}

void clean_up()
{
	using namespace stone;
	s_Controller->IOEvents.OnCleanUp(1);
}

}
