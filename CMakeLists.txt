cmake_minimum_required(VERSION 3.0)

project(audio_be)

if (NOT XEN_INCLUDE_PATH)
    message(FATAL_ERROR "XEN_INCLUDE_PATH is not defined")
endif()

if (NOT XEN_LIB_PATH)
    message(FATAL_ERROR "XEN_LIB_PATH is not defined")
endif()

set(SOURCES
	src/alsa_be.cpp
	src/BackendBase.cpp
	src/ChannelBase.cpp
	src/FrontendHandlerBase.cpp
)

include_directories(
	include
	${XEN_INCLUDE_PATH}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

link_directories(${XEN_LIB_PATH})

add_executable(audio_be ${SOURCES})

target_link_libraries(audio_be xenctrl xenstore glog pthread)

