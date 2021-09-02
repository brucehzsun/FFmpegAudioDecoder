cmake_minimum_required(VERSION 3.8.2)
project(FFmpegAudioDecoder)

set(CMAKE_C_STANDARD 11)


set(EXECUTABLE_OUTPUT_PATH ${FFmpegAudioDecoder_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${FFmpegAudioDecoder_SOURCE_DIR}/lib)

set(SRCS
        #        src/ffmpeg/main.cpp
        src/ffmpeg2/main.cpp
        )

include_directories(${FFmpegAudioDecoder_SOURCE_DIR}/include/)


IF (WIN32)
    MESSAGE(STATUS "Now is windows")
ELSEIF (APPLE)
    MESSAGE(STATUS "Now is Apple systens.")
ELSEIF (UNIX)
    MESSAGE(STATUS "Now is UNIX-like OS's.")
    include_directories(/usr/local/ffmpeg/include/)
    link_directories(/usr/local/ffmpeg/lib/)
ENDIF ()

set(LINK_DIRS avcodec avutil avformat swresample)

#add_library(FFmpegAudioDecoder SHARED ${SRCS})
add_executable(FFmpegAudioDecoder ${SRCS})
target_link_libraries(FFmpegAudioDecoder ${LINK_DIRS})