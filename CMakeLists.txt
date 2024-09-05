cmake_minimum_required(VERSION 3.25.0)

project(clar LANGUAGES C)

option(BUILD_TESTS "Build test executable" ON)

add_library(clar INTERFACE)
target_sources(clar INTERFACE
	clar.c
	clar.h
	clar/fixtures.h
	clar/fs.h
	clar/print.h
	clar/sandbox.h
	clar/summary.h
)
set_target_properties(clar PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS OFF
)

if(BUILD_TESTS)
	add_subdirectory(test)
endif()
