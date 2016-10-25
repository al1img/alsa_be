cmake_minimum_required(VERSION 3.0)

project(alsa_be)

if (NOT XEN_INCLUDE_PATH)
    message(FATAL_ERROR "XEN_INCLUDE_PATH is not defined")
endif()

if (NOT XEN_LIB_PATH)
    message(FATAL_ERROR "XEN_LIB_PATH is not defined")
endif()

if (NOT IF_INCLUDE_PATH)
    message(FATAL_ERROR "IF_INCLUDE_PATH is not defined")
endif()

set(SOURCES
	src/xen/BackendBase.cpp
	src/xen/DataChannelBase.cpp
	src/xen/EventChannel.cpp
	src/xen/FrontendHandlerBase.cpp
	src/xen/RingBuffer.cpp
	src/xen/XenStat.cpp
	src/xen/XenStore.cpp
	src/AlsaBackend.cpp
)

include_directories(
	src
	src/xen
	${XEN_INCLUDE_PATH}
	${IF_INCLUDE_PATH}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

link_directories(${XEN_LIB_PATH})

add_executable(alsa_be ${SOURCES})

target_link_libraries(alsa_be xenctrl xenstore glog pthread)

