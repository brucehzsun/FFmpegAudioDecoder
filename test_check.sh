#!/bin/bash

#valgrind --tool=memcheck --leak-check=full ./bin/FFmpegAudioDecoder > log.txt 2>&1
valgrind --tool=memcheck --leak-check=full -s --show-leak-kinds=all --log-file="logs/valgrind.txt" --track-origins=yes \
  ./bin/FFmpegAudioDecoder
