cmake_minimum_required(VERSION 3.0)

project(audio_be)

if (NOT 	)
    message(FATAL_ERROR "XEN_INCLUDE_PATH is not defined")
endif()

if (NOT XEN_LIB_PATH)
    message(FATAL_ERROR "XEN_LIB_PATH is not defined")
endif()

if (NOT IF_INCLUDE_PATH)
    message(FATAL_ERROR "IF_INCLUDE_PATH is not defined")
endif()

set(SOURCES
	src/alsa_be.cpp
	src/BackendBase.cpp
	src/DataChannelBase.cpp
	src/EventChannel.cpp
	src/FrontendHandlerBase.cpp
	src/RingBuffer.cpp
	src/XenStore.cpp
)

include_directories(
	include
	${XEN_INCLUDE_PATH}
	${IF_INCLUDE_PATH}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

link_directories(${XEN_LIB_PATH})

add_executable(audio_be ${SOURCES})

target_link_libraries(audio_be xenctrl xenstore glog pthread)

