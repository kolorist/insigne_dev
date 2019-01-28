#include <floral.h>
#include <helich.h>
#include <clover.h>

#include "Memory/MemorySystem.h"
#include "PBRTSceneDefs.h"

extern int yylex_pbrtv3(const char* i_input, const baker::pbrt::SceneCreationCallbacks& i_callbacks);

void OnNewMesh(const baker::Vec3Array& i_positions, const baker::Vec3Array& i_normals, const baker::Vec2Array& i_uvs)
{
	CLOVER_INFO("new mesh!!!");
}

int main(int argc, char** argv)
{
	// we have to call it ourself as we do not have calyx here
	helich::init_memory_system();

	clover::Initialize();
	clover::InitializeVSOutput("vs", clover::LogLevel::Verbose);
	clover::InitializeConsoleOutput("console", clover::LogLevel::Verbose);

	CLOVER_INFO("Model Baker v2");

	cstr buffer = nullptr;
	{
		floral::file_info pbrtFile = floral::open_file("scene.pbrt");
		buffer = (cstr)baker::g_PersistanceAllocator.allocate(pbrtFile.file_size + 1);
		floral::read_all_file(pbrtFile, buffer);
		buffer[pbrtFile.file_size] = 0;
		floral::close_file(pbrtFile);
	}

	baker::pbrt::SceneCreationCallbacks callbacks;
	callbacks.OnNewMesh.bind<&OnNewMesh>();

	yylex_pbrtv3(buffer, callbacks);

	return 0;
}
