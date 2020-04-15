#include <floral.h>
#include <helich.h>
#include <clover.h>
#include <stdio.h>

#include "Memory/MemorySystem.h"

int main(int argc, char** argv)
{
	// we have to call it ourself as we do not have calyx here
	helich::init_memory_system();

	clover::Initialize("main", clover::LogLevel::Verbose);
	clover::InitializeVSOutput("vs", clover::LogLevel::Verbose);
	clover::InitializeConsoleOutput("console", clover::LogLevel::Verbose);

	CLOVER_INFO("Model Baker v3");

	return 0;
}
