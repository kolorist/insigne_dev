#include "LuaScripting.h"

#include <clover/logger.h>
#include <clover/sink_topic.h>
#include <insigne/ut_render.h>

#include <floral/containers/text.h>

#include "InsigneImGui.h"
#include "lua.hpp"

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------

static FreelistArena*							s_LuaArena = nullptr;

void* custom_lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	if (nsize == 0)
	{
		if (ptr != nullptr)
		{
			s_LuaArena->free(ptr);
		}
	}
	else
	{
		if (ptr == nullptr)
		{
			return s_LuaArena->allocate(nsize);
		}
		else
		{
			return s_LuaArena->reallocate(ptr, nsize);
		}
	}
	return nullptr;
}

s32 custom_lua_print(lua_State* L)
{
	LOG_TOPIC("lua");
	s32 nArgs = lua_gettop(L);
	c8 buffer[2048];
	buffer[0] = 0;
	for (s32 i = 1; i <= nArgs; i++)
	{
		strcat(buffer, lua_tostring(L, i));
	}
	CLOVER_VERBOSE(buffer);
	return 0;
}

//-------------------------------------------------------------------

LuaScripting::LuaScripting()
{
}

LuaScripting::~LuaScripting()
{
}

ICameraMotion* LuaScripting::GetCameraMotion()
{
	return nullptr;
}

const_cstr LuaScripting::GetName() const
{
	return k_name;
}

void LuaScripting::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	floral::relative_path wdir = floral::build_relative_path("tests/perf/scripting");
	floral::push_directory(m_FileSystem, wdir);

	s_LuaArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(4));

	lua_State* luaState = lua_newstate(custom_lua_alloc, nullptr);
	luaopen_base(luaState);
	luaopen_table(luaState);
	luaopen_string(luaState);
	luaopen_math(luaState);

	struct luaL_Reg hookedFunctions[] = {
		{ "print", custom_lua_print },
		{ nullptr, nullptr }
	};
	lua_getglobal(luaState, "_G");
	luaL_setfuncs(luaState, hookedFunctions, 0);
	lua_pop(luaState, 1);
	const s32 ret = luaL_dostring(luaState, "print(\"hello world\")");
	if (ret != LUA_OK)
	{
		CLOVER_ERROR("Lua error: %s", lua_tostring(luaState, -1));
		lua_pop(luaState, 1);
	}

	lua_close(luaState);
}

void LuaScripting::_OnUpdate(const f32 i_deltaMs)
{
}

void LuaScripting::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void LuaScripting::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);

	g_StreammingAllocator.free(s_LuaArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------
}
}
