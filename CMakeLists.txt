# CMakeList.txt : CMake project for NES-Emulator, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("NES-Emulator")

# Add source to this project's executable.
add_executable (NES-Emulator "NES-Emulator.cpp" "NES-Emulator.h" "cpu.h" "cpu.cpp" "mos6502/mos6502.cpp" "olcNES/Part#2 - CPU/Bus.cpp" "olcNES/Part#2 - CPU/olc6502.cpp" "log.h" "memory.h")

#file(GLOB MY_FILES "${CMAKE_CURRENT_SOURCE_DIR}/rom/*.nes") 
#file(COPY ${MY_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms/nestest.nes" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms/nestest.log" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


# TODO: Add tests and install targets if needed.
