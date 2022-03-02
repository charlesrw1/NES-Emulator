# CMakeList.txt : CMake project for NES-Emulator, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("NES-Emulator")

# Add source to this project's executable.
add_executable (NES-Emulator "main.cpp" "main.h" "cpu.h" "cpu.cpp" "olcNES/Part#2 - CPU/Bus.cpp"
"olcNES/Part#2 - CPU/olc6502.cpp" "log.h" "mainram.h" "mainram.cpp" "cartridge.h" "ppu.h" "ppuram.h" "cartridge.cpp" "mapper.h" "emulator.h" "video_screen.h" "mapper0.h" "ppu.cpp" "ppuram.cpp" "emulator.cpp")

find_package(SFML 2.5 COMPONENTS system window graphics network audio REQUIRED)
target_link_libraries(NES-Emulator PRIVATE sfml-system sfml-network sfml-graphics sfml-window)

#file(GLOB MY_FILES "${CMAKE_CURRENT_SOURCE_DIR}/rom/*.nes") 
#file(COPY ${MY_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms/nestest.nes" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms/donkey_kong.nes" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms/smb.nes" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms/nestest.log" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# TODO: Add tests and install targets if needed.
