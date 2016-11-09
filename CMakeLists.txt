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
	src/alsa/AlsaPcm.cpp
	src/xen/BackendBase.cpp
	src/xen/FrontendHandlerBase.cpp
	src/xen/Log.cpp
	src/xen/Utils.cpp
	src/xen/XenCtrl.cpp
	src/xen/XenEvtchn.cpp
	src/xen/XenGnttab.cpp
	src/xen/XenStat.cpp
	src/xen/XenStore.cpp
	src/AlsaBackend.cpp
	src/CommandHandler.cpp
)

include_directories(
	src
	src/alsa
	src/xen
	${XEN_INCLUDE_PATH}
	${IF_INCLUDE_PATH}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

link_directories(${XEN_LIB_PATH})

add_executable(alsa_be ${SOURCES})

target_link_libraries(alsa_be xenctrl xenstore xenevtchn xengnttab asound pthread)

find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen.cfg ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg @ONLY)
	add_custom_target(
		doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
endif(DOXYGEN_FOUND)
