cmake_minimum_required(VERSION 2.6)

set(PN "tstoimg")
project(${PN} CXX)
set(CMAKE_CXX_STANDARD 11)

# option that disables AVI output if ON
option(DISABLE_VIDEO_OUTPUT "Disable VIDEO output" OFF)

set(HEADERS
	rgbfile.h
	bcellfile.h
	bsv3file.h)

set(SOURCES
	tstoimg.cpp
	rgbfile.cpp
	bcellfile.cpp
	bsv3file.cpp)

# QT
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Core Gui)

# FFMPEG
if (DISABLE_VIDEO_OUTPUT)
	add_executable(${PN} ${HEADERS} ${SOURCES})
        target_compile_definitions(${PN} PRIVATE DISABLE_VIDEO_OUTPUT=1)
else()
	set(HEADERS ${HEADERS} "QVideoEncoder.h")
	set(SOURCES ${SOURCES} "QVideoEncoder.cpp")
	add_executable(${PN} ${HEADERS} ${SOURCES})
        target_compile_definitions(${PN} PRIVATE DISABLE_VIDEO_OUTPUT=0)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
		libavformat
		libavcodec
		libswscale
		libavutil)
	target_link_libraries(${PN} PRIVATE PkgConfig::LIBAV)
endif()
unset(DISABLE_VIDEO_OUTPUT CACHE)

target_link_libraries(${PN} PRIVATE Qt5::Core)
target_link_libraries(${PN} PRIVATE Qt5::Gui)
