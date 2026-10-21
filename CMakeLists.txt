# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.16)
project(tts-phonemizer-espeak C)

# espeak-ng source tree, built with CMake in ${ESPEAK_NG_DIR}/build (static library)
set(ESPEAK_NG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../espeak-ng" CACHE PATH "espeak-ng source directory")
set(ESPEAK_NG_BUILD "${ESPEAK_NG_DIR}/build" CACHE PATH "espeak-ng build directory")

add_executable(tts-phonemizer-espeak main.c)
target_include_directories(tts-phonemizer-espeak PRIVATE "${ESPEAK_NG_DIR}/src/include")
target_link_libraries(tts-phonemizer-espeak PRIVATE
    "${ESPEAK_NG_BUILD}/src/libespeak-ng/libespeak-ng.a"
    "${ESPEAK_NG_BUILD}/src/ucd-tools/libucd.a"
    m pthread)
set_property(TARGET tts-phonemizer-espeak PROPERTY C_STANDARD 11)
target_compile_definitions(tts-phonemizer-espeak PRIVATE _GNU_SOURCE)
