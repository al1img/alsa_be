cmake_minimum_required(VERSION 3.5)

project(audio_be)

set(SOURCES
	src/audio_be.cpp
	src/BEBase.cpp
	src/FEHandlerBase.cpp
)

include_directories(
	include
	../sysroot_lp174/usr/include
)

link_directories(/home/al1/Projects/sysroot_lp174/usr/lib)

add_executable(audio_be ${SOURCES})

target_link_libraries(audio_be xenctrl xenstore glog)

target_compile_features(audio_be PRIVATE cxx_range_for)