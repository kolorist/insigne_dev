#include "shapegen.h"

#include <stdio.h>
#include <stdlib.h>

namespace shapegen
{

static generator_io_t							s_generator_io;

//----------------------------------------------

void* default_alloc(size_t i_size);
void default_free(void* i_ptr);

//----------------------------------------------

generator_io_t& get_io()
{
	return s_generator_io;
}

//----------------------------------------------

void initialize()
{
	s_generator_io.alloc = &default_alloc;
	s_generator_io.free = &default_free;
}

//----------------------------------------------

void* default_alloc(size_t i_size)
{
	return malloc(i_size);
}

void default_free(void* i_ptr)
{
	free(i_ptr);
}

}
