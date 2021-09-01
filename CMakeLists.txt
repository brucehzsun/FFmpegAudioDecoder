cmake_minimum_required(VERSION 3.8.2)
project(FFmpegAudioDecoder)

set(CMAKE_C_STANDARD 11)


set(EXECUTABLE_OUTPUT_PATH ${FFmpegAudioDecoder_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${FFmpegAudioDecoder_SOURCE_DIR}/lib)

set(SRCS
        src/ffmpeg/main.cpp
        )

include_directories(${FFmpegAudioDecoder_SOURCE_DIR}/include/)
include_directories(/usr/local/ffmpeg/include/)
link_directories(/usr/local/ffmpeg/lib/)

IF (WIN32)
    MESSAGE(STATUS "Now is windows")
    set(LINK_DIRS avcodec avutil avformat swresample)
ELSEIF (APPLE)
    MESSAGE(STATUS "Now is Apple systens.")
    set(LINK_DIRS avcodec avutil avformat swresample)
ELSEIF (UNIX)
    MESSAGE(STATUS "Now is UNIX-like OS's.")

    set(LIB_PATH "/usr/local/ffmpeg/lib/")
    #    set(LINK_DIRS ${LIB_PATH}/libavcodec.so ${LIB_PATH}/libavutil.so ${LIB_PATH}/libavformat ${LIB_PATH}/libswresample ${LIB_PATH}/libSpeex)
    set(LINK_DIRS avcodec avutil avformat swresample)
ENDIF ()

#add_library(FFmpegAudioDecoder SHARED ${SRCS})
add_executable(FFmpegAudioDecoder ${SRCS})
target_link_libraries(FFmpegAudioDecoder ${LINK_DIRS})