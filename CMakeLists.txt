cmake_minimum_required(VERSION 3.8.2)
project(FFmpegAudioDecoder)

set(CMAKE_C_STANDARD 11)


set(EXECUTABLE_OUTPUT_PATH ${FFmpegAudioDecoder_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${FFmpegAudioDecoder_SOURCE_DIR}/lib)

set(SRCS
        #        src/ffmpeg/main.cpp
        src/ffmpeg2/main.cpp
        )

include_directories(${FFmpegAudioDecoder_SOURCE_DIR}/include/ /Users/brucesun/opt/anaconda3/include)

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)

find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)

find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_library(AVUTIL_LIBRARY avutil)

find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
find_library(AVDEVICE_LIBRARY avdevice)


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