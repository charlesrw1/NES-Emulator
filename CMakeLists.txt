# CMakeList.txt : CMake project for NES-Emulator, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("NES-Emulator")

file(GLOB SOURCES
    "${PROJECT_SOURCE_DIR}/src/*.cpp")
include_directories("${PROJECT_SOURCE_DIR}/src/")



# Add source to this project's executable.
add_executable (NES-Emulator ${SOURCES})

find_package(SFML 2.5 COMPONENTS system window graphics network audio REQUIRED)
target_link_libraries(NES-Emulator PRIVATE sfml-system sfml-network sfml-graphics sfml-window)

#file(GLOB MY_FILES "${CMAKE_CURRENT_SOURCE_DIR}/rom/*.nes") 
#file(COPY ${MY_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#file(GLOB ROM_FILES "${CMAKE_CURRENT_SOURCE_DIR}/roms/")
#file(COPY ${ROM_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms/nestest.nes" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms/donkey_kong.nes" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms/smb.nes" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/roms/nestest.log" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# TODO: Add tests and install targets if needed.
