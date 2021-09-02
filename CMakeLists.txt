cmake_minimum_required(VERSION 3.8.2)
project(FFmpegAudioDecoder)

set(Boost_USE_STATIC_LIBS ON)
find_package(LibXml2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(ICONV MODULE REQUIRED)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if (COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif (COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else ()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")

endif ()

set(EXECUTABLE_OUTPUT_PATH ${FFmpegAudioDecoder_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${FFmpegAudioDecoder_SOURCE_DIR}/lib)

set(SRCS
        #        src/ffmpeg/main.cpp
        #                src/ffmpeg2/example.cpp
        src/ffmpeg/feed/main.cpp
        src/ffmpeg/feed/AudioDecoder.cpp
        src/ffmpeg/feed/AudioDecoder.h
        )

#include_directories(${FFmpegAudioDecoder_SOURCE_DIR})


IF (WIN32)
    MESSAGE(STATUS "Now is windows")
ELSEIF (APPLE)
    MESSAGE(STATUS "Now is Apple systens.")
    include_directories(/Users/brucesun/opt/anaconda3/include)
    link_directories(/Users/brucesun/opt/anaconda3/lib/)
ELSEIF (UNIX)
    MESSAGE(STATUS "Now is UNIX-like OS's.")
    include_directories(/usr/local/ffmpeg/include/)
    link_directories(/usr/local/ffmpeg/lib/)
ENDIF ()

set(LINK_DIRS avcodec avutil avformat swresample)

#add_library(FFmpegAudioDecoder SHARED ${SRCS})
add_executable(FFmpegAudioDecoder ${SRCS})
target_link_libraries(FFmpegAudioDecoder ${LINK_DIRS})