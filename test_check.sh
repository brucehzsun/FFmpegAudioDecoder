#!/bin/bash

valgrind --tool=memcheck --leak-check=full ./bin/FFmpegAudioDecoder > log.txt 2>&1
